// this is for emacs file handling -*- mode: c++; indent-tabs-mode: nil -*-

// -- BEGIN LICENSE BLOCK ----------------------------------------------
// -- END LICENSE BLOCK ------------------------------------------------

//----------------------------------------------------------------------
/*!\file
 *
 * \author  Lars Pfotzer
 * \date    2014-02-17
 * \date    2014-07-16
 *
 * This file contains the FeedBackpollingthread.
 * The hardware itself will not send data all the time, only once in response
 * to each packet sent. As the desired behaviour is to get constant position feedback
 * of the fingers we trigger a controllerfeedback periodically to receive continious data.
 * The feedback polling thread is implemented in this file.
 */
//----------------------------------------------------------------------
#include <driver_s5fh/Logging.h>
#include <driver_s5fh/S5FHFingerManager.h>

namespace driver_s5fh {

S5FHFeedbackPollingThread::S5FHFeedbackPollingThread(const TimeSpan& period, S5FHFingerManager* finger_manager)
  : PeriodicThread("S5FHReceiveThread", period),
    m_finger_manager(finger_manager)
{
}

void S5FHFeedbackPollingThread::run()
{
  while (execute())
  {
    if (m_finger_manager != NULL)
    {
      if (m_finger_manager->isConnected())
      {
        m_finger_manager->requestControllerFeedbackAllChannels();

        // This would inform a websocket server about new states, disregard that.
        #ifdef _IC_BUILDER_ICL_COMM_WEBSOCKET_
        m_finger_manager->updateWebSocket();
        #endif // _IC_BUILDER_ICL_COMM_WEBSOCKET_
      }
      else
      {
        LOGGING_WARNING_C(DriverS5FH, S5FHFeedbackPollingThread, "SCHUNK five finger hand is not connected!" << endl);
      }
    }
    else
    {
      LOGGING_WARNING_C(DriverS5FH, S5FHFeedbackPollingThread, "Pointer to FingerManager is NULL!" << endl);
    }

    // Wait for the thread period so that the timing is in sync.
    waitPeriod();
  }
}

}
