// Copyright (c) 2018-2019 All Rights Reserved WestBot

#ifndef WESTBOT_ROBOTROCK_STRATEGYMANAGER_HPP_
#define WESTBOT_ROBOTROCK_STRATEGYMANAGER_HPP_

#include <memory>

#include <QObject>

#include "ArmsManager.hpp"
#include "Common.hpp"
#include "Odometry.hpp"
#include "OpponentDetection.hpp"
#include "Recalage.hpp"
#include "TrajectoryManager.hpp"

namespace WestBot {
namespace RobotRock {

/*!
 * \brief This class is an abstract class and defines what is a strategy manager
 */
class StrategyManager : public QObject
{
public:
    using Ptr = std::shared_ptr< StrategyManager >;

    virtual ~StrategyManager() override = default;

    virtual bool init(
        const Odometry::Ptr& odometry,
        const Recalage::Ptr& recalage,
        const ArmsManager::Ptr& armsManager,
        const OpponentDetection::Ptr opponentDetection,
        const TrajectoryManager::Ptr& trajectoryManager ) = 0;

    virtual bool init(
        const Odometry::Ptr& odometry,
        const TrajectoryManager::Ptr& trajectoryManager ) = 0;

    virtual void deinit() = 0;
    virtual void stop() = 0;

    virtual void doStrat( const Color& color ) = 0;

    virtual void buildStrat( const Color& color ) = 0;

    virtual void hardStop() = 0;

    virtual void obstacleAt( double xStart, double yStart, double xEnd, double zEnd ) = 0;
};

}
}

#endif // WESTBOT_ROBOTROCK_STRATEGYMANAGER_HPP_
