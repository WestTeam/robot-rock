// COPYRIGHT (C) 2019 ALL RIGHTS RESERVED WESTBot

#ifndef WESTBOT_ROBOTROCK_SYSTEMMANAGERHW_HPP_
#define WESTBOT_ROBOTROCK_SYSTEMMANAGERHW_HPP_

#include "ArmHighLevel.hpp"
#include "ArmsManager.hpp"
#include "Experiment.hpp"
#include "GameThread.hpp"
#include "Lidar.hpp"
#include "Monitoring.hpp"
#include "OpponentDetection.hpp"
#include "Recalage.hpp"
#include "PuckDetection.hpp"
#include "SimTcpServer.hpp"
#include "StrategyManager.hpp"
#include "SystemManager.hpp"
#include "TrajectoryManager.hpp"
#include "Vl6180x.hpp"

namespace WestBot {
namespace RobotRock {

class SystemManagerHw : public SystemManager
{
public:
    enum class SystemMode
    {
        Free, // No PID -> register = 0x00
        DistanceOnly, // PIDD -> register = 0x01
        AngleOnly, // PIDA -> register = 0x02
        Full, // All PID -> register = 0x03
        Unused // 0xFF : Unused for now
    };

    SystemManagerHw(
        const Hal::Ptr& hal,
        const StrategyManager::Ptr& strategyManager,
        QObject* parent = nullptr );

    ~SystemManagerHw() override;

    bool init() override;

    void start() override;
    void stop() override;
    void reset() override;

    void setMode( SystemMode mode );
    SystemMode mode() const;

    bool isSafe() const override;

private:
    void initRecalage();

protected:
    void displayColor( const DigitalValue& value ) override;

private:
    Hal::Ptr _hal;

    SystemMode _systemMode;

    // Base system
    Odometry::Ptr _odometry;
    Recalage::Ptr _recalage;
    LidarRPLidarA2::Ptr _lidarTop;
    LidarRPLidarA2::Ptr _lidarFront;
    LidarRPLidarA2::Ptr _lidarRear;
    TrajectoryManager::Ptr _trajectoryManager;
    Vl6180x _distanceSensorLeft;
    Vl6180x _distanceSensorRight;
    ArmLowLevel::Ptr _armLeftLow;
    ArmLowLevel::Ptr _armRightLow;
    ArmHighLevel::Ptr _armLeft;
    ArmHighLevel::Ptr _armRight;
    ArmsManager::Ptr _armsManager;
    PuckDetection::Ptr _puckDetection;
    OpponentDetection::Ptr _opponentDetection;
    Monitoring::Ptr _monitoring;
    GameThread::Ptr _game;
    Experiment _experiment;
    SimTcpServer _simServer;

    // Last
    StrategyManager::Ptr _strategyManager;
};

}
}

#endif // WESTBOT_ROBOTROCK_SYSTEMMANAGERHW_HPP_
