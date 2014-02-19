// this is for emacs file handling -*- mode: c++; indent-tabs-mode: nil -*-

// -- BEGIN LICENSE BLOCK ----------------------------------------------
// -- END LICENSE BLOCK ------------------------------------------------

//----------------------------------------------------------------------
/*!\file
 *
 * \author  Lars Pfotzer
 * \date    2014-01-30
 *
 */
//----------------------------------------------------------------------
#include <driver_s5fh/Logging.h>
#include <driver_s5fh/S5FHFingerManager.h>

#include <icl_core/TimeStamp.h>

namespace driver_s5fh {

S5FHFingerManager::S5FHFingerManager() :
  m_controller(new S5FHController()),
  m_feedback_thread(NULL),
  m_connected(false),
  m_homing_timeout(10),
  m_home_settings(0),
  m_ticks2rad(0),
  m_position_min(std::vector<int32_t>(eS5FH_DIMENSION, 0)),
  m_position_max(std::vector<int32_t>(eS5FH_DIMENSION, 0)),
  m_position_home(std::vector<int32_t>(eS5FH_DIMENSION, 0)),
  m_is_homed(std::vector<bool>(eS5FH_DIMENSION, false))
{
  // load home position default parameters
  setHomePositionDefaultParameters();
}

S5FHFingerManager::~S5FHFingerManager()
{
  disconnect();

  if (m_controller != NULL)
  {
    delete m_controller;
    m_controller = NULL;
  }
}

bool S5FHFingerManager::connect(const std::string &dev_name)
{
  if (m_connected)
  {
    disconnect();
  }

  if (m_controller != NULL)
  {
    if (m_controller->connect(dev_name))
    {
      // initialize and start feedback polling thread
      m_feedback_thread = new S5FHFeedbackPollingThread(icl_core::TimeSpan::createFromMSec(100), this);
      if (m_feedback_thread != NULL)
      {
        m_feedback_thread->start();
      }

      // load default position settings
      std::vector<S5FHPositionSettings> default_position_settings
          = getPositionSettingsDefaultParameters();

      // load default current settings
      std::vector<S5FHCurrentSettings> default_current_settings
          = getCurrentSettingsDefaultParameters();

      m_controller->disableChannel(eS5FH_ALL);

      // initialize all channels
      for (size_t i = 0; i < eS5FH_DIMENSION; ++i)
      {
        // request controller feedback
        m_controller->requestControllerFeedback(static_cast<S5FHCHANNEL>(i));

        // set position settings
        m_controller->setPositionSettings(static_cast<S5FHCHANNEL>(i), default_position_settings[i]);

        // set current settings
        m_controller->setCurrentSettings(static_cast<S5FHCHANNEL>(i), default_current_settings[i]);
      }

      // check for correct response from hardware controller
      icl_core::TimeStamp start_time = icl_core::TimeStamp::now();
      bool timeout = false;
      while (!timeout && !m_connected)
      {
        unsigned int send_count = m_controller->getSentPackageCount();
        unsigned int received_count = m_controller->getReceivedPackageCount();
        if (send_count == received_count)
        {
          m_connected = true;
          LOGGING_INFO_C(DriverS5FH, S5FHFingerManager, "Successfully established connection to SCHUNK five finger hand." << endl
                          << "Send packages = " << send_count << ", received packages = " << received_count << endl);
        }
        LOGGING_DEBUG_C(DriverS5FH, S5FHFingerManager, "Try to connect to SCHUNK five finger hand: Send packages = " << send_count << ", received packages = " << received_count << endl);

        // check for timeout
        if ((icl_core::TimeStamp::now() - start_time).tsSec() > 5.0)
        {
          timeout = true;
          LOGGING_ERROR_C(DriverS5FH, S5FHFingerManager, "Connection timeout! Could not connect to SCHUNK five finger hand." << endl
                          << "Send packages = " << send_count << ", received packages = " << received_count << endl);
        }

        icl_core::os::usleep(50000);
      }
    }
  }

  return m_connected;
}

void S5FHFingerManager::disconnect()
{
  m_connected = false;

  if (m_feedback_thread != NULL)
  {
    // wait until thread has stopped
    m_feedback_thread->stop();
    m_feedback_thread->join();

    delete m_feedback_thread;
    m_feedback_thread = NULL;
  }

  if (m_controller != NULL)
  {
    m_controller->disconnect();
  }
}

//! reset function for a single finger
bool S5FHFingerManager::resetChannel(const S5FHCHANNEL &channel)
{
  if (m_connected)
  {
    // reset all channels
    if (channel == eS5FH_ALL)
    {
      bool reset_all_success = true;
      for (size_t i = 0; i < eS5FH_DIMENSION; ++i)
      {
        // try three times to reset each finger
        size_t max_reset_counter = 3;
        bool reset_success = false;
        while (!reset_success && max_reset_counter > 0)
        {
          reset_success = resetChannel(static_cast<S5FHCHANNEL>(i));
          max_reset_counter--;

          // wait before starting next reset
          icl_core::os::sleep(1);
        }

        LOGGING_INFO_C(DriverS5FH, resetChannel, "Channel " << i << " reset success = " << reset_success << endl);

        // set all reset flag
        reset_all_success = reset_all_success && reset_success;
      }
      return reset_all_success;
    }
    else if (channel > eS5FH_ALL && eS5FH_ALL < eS5FH_DIMENSION)
    {
      LOGGING_DEBUG_C(DriverS5FH, resetChannel, "Start homing channel " << channel << endl);

      // reset homed flag
      m_is_homed[channel] = false;

      // read default home settings for channel
      HomeSettings home = m_home_settings[channel];

      S5FHPositionSettings pos_set;
      S5FHCurrentSettings cur_set;
      m_controller->getPositionSettings(channel, pos_set);
      m_controller->getCurrentSettings(channel, cur_set);

      // find home position
      m_controller->disableChannel(eS5FH_ALL);
      int32_t position = 0;

      if (home.direction > 0)
      {
        position = static_cast<int32_t>(pos_set.wmx);
      }
      else
      {
        position = static_cast<int32_t>(pos_set.wmn);
      }
      m_controller->setControllerTarget(channel, position);
      m_controller->enableChannel(channel);

      S5FHControllerFeedback control_feedback_previous;
      S5FHControllerFeedback control_feedback;

      // initialize timeout
      icl_core::TimeStamp start_time = icl_core::TimeStamp::now();

      for (size_t hit_count = 0; hit_count < 10; )
      {
        m_controller->setControllerTarget(channel, position);
        //m_controller->requestControllerFeedback(channel);
        m_controller->getControllerFeedback(channel, control_feedback);

        if ((0.75 * cur_set.wmn >= control_feedback.current) || (control_feedback.current >= 0.75 * cur_set.wmx))
        {
          hit_count++;
        }
        else if (hit_count > 0)
        {
          hit_count--;
        }

        // check for time out: Abort, if position does not change after homing timeout.
        if ((icl_core::TimeStamp::now() - start_time).tsSec() > m_homing_timeout)
        {
          m_controller->disableChannel(eS5FH_ALL);
          LOGGING_ERROR_C(DriverS5FH, resetChannel, "Timeout: Aborted finding home position for channel " << channel << endl);
          return false;
        }

        // reset time of position changes
        if (control_feedback.position != control_feedback_previous.position)
        {
          start_time = icl_core::TimeStamp::now();
        }

        // save previous control feedback
        control_feedback_previous = control_feedback;
      }

      LOGGING_DEBUG_C(DriverS5FH, resetChannel, "Hit counter of " << channel << " reached." << endl);

      m_controller->disableChannel(eS5FH_ALL);

      // set reference values
      m_position_min[channel] = static_cast<int32_t>(control_feedback.position + home.minimumOffset);
      m_position_max[channel] = static_cast<int32_t>(control_feedback.position + home.maximumOffset);
      m_position_home[channel] = static_cast<int32_t>(control_feedback.position + home.idlePosition);
      LOGGING_DEBUG_C(DriverS5FH, resetChannel, "Channel " << channel << " min pos = " << m_position_min[channel]
                      << " max pos = " << m_position_max[channel] << " home pos = " << m_position_home[channel] << endl);

      position = static_cast<int32_t>(control_feedback.position + home.idlePosition);

      // go to idle position
      m_controller->enableChannel(channel);
      while (true)
      {
        m_controller->setControllerTarget(channel, position);
        //m_controller->requestControllerFeedback(channel);
        m_controller->getControllerFeedback(channel, control_feedback);

        if (abs(position - control_feedback.position) < 1000)
        {
          break;
        }
      }
      m_controller->disableChannel(eS5FH_ALL);

      m_is_homed[channel] = true;

      LOGGING_DEBUG_C(DriverS5FH, resetChannel, "End homing of channel " << channel << endl);

      return true;
    }
    else
    {
      LOGGING_ERROR_C(DriverS5FH, resetChannel, "Could not reset channel " << channel << ": No connection to SCHUNK five finger hand!" << endl);
      return false;
    }
  }
  else
  {
    LOGGING_ERROR_C(DriverS5FH, resetChannel, "Channel " << channel << " is out side of bounds!" << endl);
    return false;
  }
}

//! enables controller of channel
bool S5FHFingerManager::enableChannel(const S5FHCHANNEL &channel)
{
  if (isConnected() && isHomed(channel))
  {
    m_controller->enableChannel(channel);
    return true;
  }

  return false;
}

//! disables controller of channel
void S5FHFingerManager::disableChannel(const S5FHCHANNEL &channel)
{
  m_controller->disableChannel(channel);
}

bool S5FHFingerManager::requestControllerFeedback(const S5FHCHANNEL &channel)
{
  if (isConnected() && isHomed(channel) && isEnabled(channel))
  {
    m_controller->requestControllerFeedback(channel);
    return true;
  }

  LOGGING_WARNING_C(DriverS5FH, S5FHFingerManager, "Channel " << channel << " is not homed or is not enabled!" << endl);
  return false;
}

//! returns actual position value for given channel
bool S5FHFingerManager::getPosition(const S5FHCHANNEL &channel, double &position)
{
  S5FHControllerFeedback controller_feedback;
  if (isHomed(channel) && m_controller->getControllerFeedback(channel, controller_feedback))
  {
    int32_t cleared_position_ticks = controller_feedback.position;

    if (m_home_settings[channel].direction > 0)
    {
      cleared_position_ticks -= m_position_max[channel];
    }
    else
    {
      cleared_position_ticks -= m_position_min[channel];
    }

    position = static_cast<double>(cleared_position_ticks * m_ticks2rad[channel]);


    LOGGING_DEBUG_C(DriverS5FH, S5FHFingerManager, "Channel " << channel << ": position_ticks = " << controller_feedback.position
                    << " | cleared_position_ticks = " << cleared_position_ticks << " | position rad = " << position << endl);
    return true;
  }
  else
  {
    LOGGING_WARNING_C(DriverS5FH, S5FHFingerManager, "Could not get postion for channel " << channel << endl);
    return false;
  }
}

//! returns actual current value for given channel
bool S5FHFingerManager::getCurrent(const S5FHCHANNEL &channel, double &current)
{
  S5FHControllerFeedback controller_feedback;
  if (isHomed(channel) && m_controller->getControllerFeedback(channel, controller_feedback))
  {
    current = controller_feedback.current;
    return true;
  }
  else
  {
    LOGGING_WARNING_C(DriverS5FH, setTargetPosition, "Could not get current for channel " << channel << endl);
    return false;
  }
}


//! set target position of a single finger
bool S5FHFingerManager::setTargetPosition(const S5FHCHANNEL &channel, double position, double current)
{
  if (isConnected())
  {
    if (isHomed(channel))
    {
      int32_t target_position = static_cast<int32_t>(position / m_ticks2rad[channel]);

      if (m_home_settings[channel].direction > 0)
      {
        target_position += m_position_max[channel];
      }
      else
      {
        target_position += m_position_min[channel];
      }

      LOGGING_DEBUG_C(DriverS5FH, setTargetPosition, "Target position for channel " << channel << " = " << target_position << endl);

      // check for bounds
      if (target_position >= m_position_min[channel] && target_position <= m_position_max[channel])
      {
        if (!m_controller->isEnabled(channel))
        {
          m_controller->enableChannel(channel);
        }

        m_controller->setControllerTarget(channel, target_position);
        return true;
      }
      else
      {
        LOGGING_ERROR_C(DriverS5FH, setTargetPosition, "Target position for channel " << channel << " out of bounds!" << endl);
        return false;
      }
    }
    else
    {
      LOGGING_ERROR_C(DriverS5FH, setTargetPosition, "Could not set target position for channel " << channel << ": Reset first!" << endl);
      return false;
    }
  }
  else
  {
    LOGGING_ERROR_C(DriverS5FH, setTargetPosition, "Could not set target position for channel " << channel << ": No connection to SCHUNK five finger hand!" << endl);
    return false;
  }
}

//! overwrite current parameters
bool S5FHFingerManager::setCurrentControllerParams(const S5FHCHANNEL &channel, const S5FHCurrentSettings &current_settings)
{
  m_controller->setCurrentSettings(channel, current_settings);
  return true;
}

//! overwrite position parameters
bool S5FHFingerManager::setPositionControllerParams(const S5FHCHANNEL &channel, const S5FHPositionSettings &position_settings)
{
  m_controller->setPositionSettings(channel, position_settings);
  return true;
}

//! return enable flag
bool S5FHFingerManager::isEnabled(const S5FHCHANNEL &channel)
{
  return m_controller->isEnabled(channel);
}

//! return homed flag
bool S5FHFingerManager::isHomed(const S5FHCHANNEL &channel)
{
  return m_is_homed[channel];
}

//!
//! \brief set default parameters for home position
//!
void S5FHFingerManager::setHomePositionDefaultParameters()
{
  m_home_settings.resize(eS5FH_DIMENSION);
                    // direction, minimum offset, maximum offset, idle position
  HomeSettings home_set_thumb_flexion   = {+1, -175.0e3f,  -5.0e3f, -15.0e3f};  // RE17, thumb flexion
  HomeSettings home_set_thumb_oppsition = {+1, -105.0e3f,  -5.0e3f, -15.0e3f};  // RE17, thumb opposition
  HomeSettings home_set_finger_distal   = {+1,  -47.0e3f,  -2.0e3f,  -8.0e3f};  // RE10, index finger distal joint
  HomeSettings home_set_finger_proximal = {-1,    2.0e3f,  42.0e3f,   8.0e3f};  // {-1,    2.0e3f,  47.0e3f,   8.0e3f}; RE13, index finger proximal joint
  HomeSettings home_set_ring_finger     = home_set_finger_distal; //{+1,  -47.0e3f,  -2.0e3f,  -8.0e3f};  // RE10, ring finger
  HomeSettings home_set_pinky           = home_set_finger_distal; //{+1,  -47.0e3f,  -2.0e3f,  -8.0e3f};  // RE10, pinky
  HomeSettings home_set_finger_spread   = {+1,  -27.0e3f,  -2.0e3f,  -8.0e3f};  // {+1,  -47.0e3f,  -2.0e3f,  -8.0e3f};  // RE13, finger spread

  m_home_settings[0] = home_set_thumb_flexion;    // thumb flexion
  m_home_settings[1] = home_set_thumb_oppsition;  // thumb opposition
  m_home_settings[2] = home_set_finger_distal;    // index finger distal joint
  m_home_settings[3] = home_set_finger_proximal;  // index finger proximal joint
  m_home_settings[4] = home_set_finger_distal;    // middle finger distal joint
  m_home_settings[5] = home_set_finger_proximal;  // middle finger proximal joint
  m_home_settings[6] = home_set_ring_finger;      // ring finger
  m_home_settings[7] = home_set_pinky;            // pinky
  m_home_settings[8] = home_set_finger_spread;    // finger spread

  // calculate factors and offset for ticks to rad conversion
  float range_rad_data[eS5FH_DIMENSION] = { 0.97, 0.99, 1.33, 0.8, 1.33, 0.8, 0.98, 0.98, 0.58 };
  std::vector<float> range_rad(&range_rad_data[0], &range_rad_data[0] + eS5FH_DIMENSION);

  m_ticks2rad.resize(eS5FH_DIMENSION, 0.0);
  for (size_t i = 0; i < eS5FH_DIMENSION; ++i)
  {
    float range_ticks = m_home_settings[i].maximumOffset - m_home_settings[i].minimumOffset;
    m_ticks2rad[i] = range_rad[i] / range_ticks * (-m_home_settings[i].direction);

    // debug
    //std::cout << "Channel " << i << ": ticks2rad factor = " << m_ticks2rad[i] << std::endl;
  }
}

//!
//! \brief returns default parameters for current settings
//!
std::vector<S5FHCurrentSettings> S5FHFingerManager::getCurrentSettingsDefaultParameters()
{
  std::vector<S5FHCurrentSettings> default_current_settings(eS5FH_DIMENSION);
  S5FHCurrentSettings cur_set_thumb          = {-191.0f, 191.0f, 0.405f, 4e-6f, -300.0f, 300.0f, 0.850f, 85.0f, -254.0f, 254.0f};
  S5FHCurrentSettings cur_set_distal_joint   = {-176.0f, 176.0f, 0.405f, 4e-6f, -300.0f, 300.0f, 0.850f, 85.0f, -254.0f, 254.0f};
  S5FHCurrentSettings cur_set_proximal_joint = {-167.0f, 167.0f, 0.405f, 4e-6f, -300.0f, 300.0f, 0.850f, 85.0f, -254.0f, 254.0f};

  default_current_settings[0] = cur_set_thumb;          // thumb flexion
  default_current_settings[1] = cur_set_thumb;          // thumb opposition
  default_current_settings[2] = cur_set_distal_joint;   // index finger distal joint
  default_current_settings[3] = cur_set_proximal_joint; // index finger proximal joint
  default_current_settings[4] = cur_set_distal_joint;   // middle finger distal joint
  default_current_settings[5] = cur_set_proximal_joint; // middle finger proximal joint
  default_current_settings[6] = cur_set_distal_joint;   // ring finger
  default_current_settings[7] = cur_set_distal_joint;   // pinky
  default_current_settings[8] = cur_set_proximal_joint; // finger spread

  return default_current_settings;
}

//!
//! \brief returns default parameters for position settings
//!
std::vector<S5FHPositionSettings> S5FHFingerManager::getPositionSettingsDefaultParameters()
{
  std::vector<S5FHPositionSettings> default_position_settings(eS5FH_DIMENSION);
  S5FHPositionSettings pos_set_thumb = {-1.0e6f, 1.0e6f,  3.4e3f, 1.00f, 1e-3f, -500.0f, 500.0f, 0.5f, 0.05f, 0.0f};
  S5FHPositionSettings pos_set_finger = {-1.0e6f, 1.0e6f,  8.5e3f, 1.00f, 1e-3f, -500.0f, 500.0f, 0.5f, 0.05f, 0.0f};
  S5FHPositionSettings pos_set_spread = {-1.0e6f, 1.0e6f, 17.0e3f, 1.00f, 1e-3f, -500.0f, 500.0f, 0.5f, 0.05f, 0.0f};

  default_position_settings[0] = pos_set_thumb;   // thumb flexion
  default_position_settings[1] = pos_set_thumb;   // thumb opposition
  default_position_settings[2] = pos_set_finger;  // index finger distal joint
  default_position_settings[3] = pos_set_finger;  // index finger proximal joint
  default_position_settings[4] = pos_set_finger;  // middle finger distal joint
  default_position_settings[5] = pos_set_finger;  // middle finger proximal joint
  default_position_settings[6] = pos_set_finger;  // ring finger
  default_position_settings[7] = pos_set_finger;  // pinky
  default_position_settings[8] = pos_set_spread;  // finger spread

  return default_position_settings;
}

bool S5FHFingerManager::readParametersFromConfigFile()
{
//  bool read_successful = false;

//  // load position settings from config file
//  std::vector<S5FHPositionSettings> position_config_list;
//  read_successful =
//    icc::get(CONFIG_VALUES(
//               CONFIG_LIST(
//                 S5FHPositionSettings, "/S5FH/PositionSettings",
//                 MEMBER_MAPPING(
//                   S5FHPositionSettings,
//                   MEMBER_VALUE_1("WMin", S5FHPositionSettings, wmn)
//                   MEMBER_VALUE_1("WMax", S5FHPositionSettings, wmx)
//                   MEMBER_VALUE_1("DWMax", S5FHPositionSettings, dwmx)
//                   MEMBER_VALUE_1("KY", S5FHPositionSettings, ky)
//                   MEMBER_VALUE_1("DT", S5FHPositionSettings, dt)
//                   MEMBER_VALUE_1("IMin", S5FHPositionSettings, imn)
//                   MEMBER_VALUE_1("IMax", S5FHPositionSettings, imx)
//                   MEMBER_VALUE_1("KP", S5FHPositionSettings, kp)
//                   MEMBER_VALUE_1("KI", S5FHPositionSettings, ki)
//                   MEMBER_VALUE_1("KD", S5FHPositionSettings, kd)),
//                 std::back_inserter(position_config_list))),
//             DriverS5FH::instance());

//  // set controller position settings
//  if (read_successful)
//  {
//    for (size_t i = 0; i < position_config_list.size(); i++)
//    {
//      m_controller->setPositionSettings(i, position_config_list[i]);

//      LOGGING_ERROR_C(DriverS5FH, S5FHController, "new position settings recieved: " << endl <<
//                      "WMin = " << position_config_list[i].wmn << endl);
//    }
//  }
//  else
//  {
//    LOGGING_ERROR_C(DriverS5FH, S5FHFingerManager, "Could not load position settings from config file" << endl);
//  }

//  // load current settings from config file
//  std::vector<S5FHCurrentSettings> current_config_list;
//  read_successful =
//    icc::get(CONFIG_VALUES(
//               CONFIG_LIST(
//                 S5FHCurrentSettings, "/S5FH/CurrentSettings",
//                 MEMBER_MAPPING(
//                   S5FHCurrentSettings,
//                   MEMBER_VALUE_1("WMin", S5FHCurrentSettings, wmn)
//                   MEMBER_VALUE_1("WMax", S5FHCurrentSettings, wmx)
//                   MEMBER_VALUE_1("KY", S5FHCurrentSettings, ky)
//                   MEMBER_VALUE_1("DT", S5FHCurrentSettings, dt)
//                   MEMBER_VALUE_1("IMin", S5FHCurrentSettings, imn)
//                   MEMBER_VALUE_1("IMax", S5FHCurrentSettings, imx)
//                   MEMBER_VALUE_1("KP", S5FHCurrentSettings, kp)
//                   MEMBER_VALUE_1("KI", S5FHCurrentSettings, ki)
//                   MEMBER_VALUE_1("UMin", S5FHCurrentSettings, umn)
//                   MEMBER_VALUE_1("UMax", S5FHCurrentSettings, umx)),
//                 std::back_inserter(current_config_list))),
//             icl_core::logging::Nirwana::instance());

//  // set current position settings
//  if (read_successful)
//  {
//    for (size_t i = 0; i < current_config_list.size(); i++)
//    {
//      m_controller->setCurrentSettings(i, current_config_list[i]);
//    }
//  }
//  else
//  {
//    LOGGING_ERROR_C(DriverS5FH, S5FHFingerManager, "Could not load current settings from config file" << endl);
//  }
  return true;
}

}
