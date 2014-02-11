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
#include <driver_s5fh/S5FHFingerManager.h>

#include <driver_s5fh/Logging.h>

namespace driver_s5fh {

S5FHFingerManager::S5FHFingerManager()
{
  //TODO: Get this string from somewhere
  m_controller = new S5FHController("/dev/tty1");

  // load home position default parameters
  setHomePositionDefaultParameters();

  // load default current settings
  setCurrentSettingsDefaultParameters();

  // load default position settings
  setPositionSettingsDefaultParameters();

}

S5FHFingerManager::~S5FHFingerManager()
{
}

//! reset function for a single finger
bool S5FHFingerManager::resetChannel(const S5FHCHANNEL &channel)
{
  /*
  HomeSettings home = m_home_settings[channel];

  S5FHPositionSettings pos_set = m_controller->getPositionSettings(channel);

  // home
  m_controller->disableChannel(eS5FH_ALL);
  u_int32_t position = 0;

  if (home.direction > 0)
  {
    position = static_cast<u_int32_t>(pos_set.wmx);
  }
  else
  {
    position = static_cast<u_int32_t>(pos_set.wmn);
  }
  m_controller->setControllerTarget(channel, position);

  m_controller->enableChannel(channel);

  for (size_t hit_count = 0; hit_count < 10; )
  {
    m_controller->setControllerTarget(channel, position);

    if ((0.75 * pos_set.wmn >= m_controller->getControllerFeedback(channel).current) || (controlFeedback.current >= 0.75 * pos_set.wmx))
    {
      hit_count++;
    }
    else if (hit_count > 0)
    {
      hit_count--;
    }
  }
  m_controller->disableChannel(eS5FH_ALL);

  // set reference values
  m_position_min = controlFeedback.position + home.minimumOffset;
  m_position_max = controlFeedback.position + home.maximumOffset;

  position = static_cast<u_int32_t>(controlFeedback.position + home.idlePosition);

  // go to idle position
  m_controller->enableChannel(channel);
  while (true)
  {
    m_controller->setControllerTarget(channel, position);

    if (Math.Abs(controlCommand.position - controlFeedback.position) < 1000)
    {
      break;
    }
  }
  m_controller->disableChannel(eS5FH_ALL);

  this.controlCommand.position = controlFeedback.position;

  IsHomed = true;
  homeThread = null;
  */
  return true;
}

//! set target position of a single finger
bool S5FHFingerManager::setTargetPosition(const S5FHCHANNEL &channel, double position, double current)
{
  //TODO: Convert position into ticks

  m_controller->setControllerTarget(channel, 23); // TODO: Replace this with something usefull
  return true;
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

//!
//! \brief set default parameters for home position
//!
void S5FHFingerManager::setHomePositionDefaultParameters()
{
  m_home_settings.resize(eS5FH_DIMENSION);
  HomeSettings home_set_thumb_flexion   = {+1, -175.0e3f,  -5.0e3f, -15.0e3f};  // RE17, thumb flexion
  HomeSettings home_set_thumb_oppsition = {+1, -105.0e3f,  -5.0e3f, -15.0e3f};  // RE17, thumb opposition
  HomeSettings home_set_finger_distal   = {+1,  -47.0e3f,  -2.0e3f,  -8.0e3f};  // RE10, index finger distal joint
  HomeSettings home_set_finger_proximal = {-1,    2.0e3f,  47.0e3f,   8.0e3f};  // RE13, index finger proximal joint
  HomeSettings home_set_ring_finger     = home_set_finger_distal; //{+1,  -47.0e3f,  -2.0e3f,  -8.0e3f};  // RE10, ring finger
  HomeSettings home_set_pinky           = home_set_finger_distal; //{+1,  -47.0e3f,  -2.0e3f,  -8.0e3f};  // RE10, pinky
  HomeSettings home_set_finger_spread   = home_set_finger_distal; //{+1,  -47.0e3f,  -2.0e3f,  -8.0e3f};  // RE13, finger spread

  m_home_settings[0] = home_set_thumb_flexion;    // thumb flexion
  m_home_settings[1] = home_set_thumb_oppsition;  // thumb opposition
  m_home_settings[2] = home_set_finger_distal;    // index finger distal joint
  m_home_settings[3] = home_set_finger_proximal;  // index finger proximal joint
  m_home_settings[4] = home_set_finger_distal;    // middle finger distal joint
  m_home_settings[5] = home_set_finger_proximal;  // middle finger proximal joint
  m_home_settings[6] = home_set_ring_finger;      // ring finger
  m_home_settings[7] = home_set_pinky;            // pinky
  m_home_settings[8] = home_set_finger_spread;    // finger spread
}

//!
//! \brief set default parameters for current settings
//!
void S5FHFingerManager::setCurrentSettingsDefaultParameters()
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

  for (size_t i = 0; i < eS5FH_DIMENSION; ++i)
  {
    m_controller->setCurrentSettings(static_cast<S5FHCHANNEL>(i), default_current_settings[i]);
  }
}

//!
//! \brief set default parameters for position settings
//!
void S5FHFingerManager::setPositionSettingsDefaultParameters()
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

  for (size_t i = 0; i < eS5FH_DIMENSION; i++)
  {
    m_controller->setPositionSettings(static_cast<S5FHCHANNEL>(i), default_position_settings[i]);
  }
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
