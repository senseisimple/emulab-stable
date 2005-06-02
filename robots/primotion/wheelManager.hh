/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2005 University of Utah and the Flux Group.
 * All rights reserved.
 */

/**
 * @file wheelManager.hh
 *
 * Header file for the wheelManager class.
 */

#ifndef _wheel_manager_hh
#define _wheel_manager_hh

#include <assert.h>

#include "aGarciaDefs.tea"
#include "acpGarcia.h"

#include "ledManager.hh"

class dashboard;
class wheelManager;

/**
 * Callback for wheelManager controlled moves/pivots.
 */
class wmCallback
{
    
 public:

  /**
   * Destructor.
   */
  virtual ~wmCallback();

  /**
   * Method called when a wheelManager controlled move/pivot finishes.
   *
   * @param status The completion status of the move.  (One of the
   * aGARCIA_ERRFLAG values.)
   */
  virtual void call(int status, float odometer) = 0;
    
};

/**
 * Arbiter for the wheels on a garcia robot.
 */
class wheelManager
{

 public:

  /**
   * The smallest angle that will be tolerated when creating a pivot
   * behavior.
   *
   * @see createPivot
   */
  static const float SMALLEST_PIVOT_ANGLE = 0.003f;

  /**
   * The smallest distance that will be tolerated when creating a move
   * behavior.
   *
   * @see createMove
   */
  static const float SMALLEST_MOVE_DISTANCE = 0.001f;

  /**
   * The minimum allowed speed in meters per second.
   *
   * @see setSpeed
   */
  static const float MINIMUM_SPEED = 0.10;

  /**
   * The default speed in meters per second.
   *
   * @see setSpeed
   */
  static const float DEFAULT_SPEED = 0.20;
    
  /**
   * The maximum allowed speed in meters per second.
   *
   * @see setSpeed
   */
  static const float MAXIMUM_SPEED = 0.40;
    
  /**
   * The minimum allowed acceleration in meters per second squared.
   */
  static const float MINIMUM_ACCELERATION = 0.05;

  /**
   * The default acceleration in meters per second squared.
   */
  static const float DEFAULT_ACCELERATION = 0.20;
    
  /**
   * The maximum allowed acceleration in meters per second squared.
   */
  static const float MAXIMUM_ACCELERATION = 4.0;
    

  /**
   * Construct a wheelManager object for a garcia.
   *
   * @param garcia The garcia, whose wheel's should be controlled.
   */
  wheelManager(acpGarcia &garcia);

  /**
   * Destructor.
   */
  virtual ~wheelManager();

  /**
   * @param dashboard The dashboard to update when moving.
   */
  void setDashboard(dashboard *dashboard)
  { this->wm_dashboard = dashboard; };

  /**
   * @param speed The wheel speed to use when moving.  Value is in meters per
   * second.  If the value is not between the min and max the default speed
   * will be used.
   */
  void setSpeed(float speed);

  /**
   * Create a "pivot" behavior that will rotate the robot to the given angle
   * in the robot's local coordinate system.  The created behavior will only
   * rotate the robot to the final angle and in the shortest manner possible.
   * In other words, an input angle of 405 degrees will rotate the robot 45
   * degrees, instead of doing a full revolution plus 45 degrees.
   *
   * @param angle The angle, in radians, to create a pivot for.
   * @param callback A callback to trigger when the pivot completes.  If no
   * behavior is created, this callback will never be triggered.
   * @return The behavior object or NULL if the angle is too small of a
   * movement.
   */
  virtual acpObject *createPivot(float angle, wmCallback *callback = NULL);

  /**
   * Create a "move" behavior that will move the robot the given distance.
   *
   * @param distance The distance, in meters, to move the robot.
   * @param callback A callback to trigger when the pivot completes.  If no
   * behavior is created, this callback will never be triggered.
   * @return The behavior object or NULL if the distance was too small.
   */
  virtual acpObject *createMove(float distance, wmCallback *callback = NULL);

  /**
   * Set the robot's destination in the local coordinate system.  This method
   * will queue the appropriate pivot and move behaviors required to move the
   * robot to the given destination and then trigger the given callback upon
   * completion.  The movement will be made in the most efficient way
   * possible, so it will rotate as much as needed and the move forward or
   * backward the appropriate amount.
   *
   * @param x The x coordinate.
   * @param y The y coordinate.
   * @param callback A callback to trigger when the movement completes.  If
   * the movement was too small, the status passed to the callback will be
   * aGARCIA_ERRFLAG_WONTEXECUTE.
   */
  virtual void setDestination(float x, float y, wmCallback *callback = NULL);

  /**
   * Set the robot's orientation in the local coordinate system. This method
   * will queue the appropriate pivot and then trigger the given callback
   * upon completion.
   *
   * @param orientation The new orientation, in radians, for the robot.
   * @param callback A callback to trigger when the movement completes.  If
   * the movement was too small, the status passed to the callback will be
   * aGARCIA_ERRFLAG_WONTEXECUTE.
   */
  virtual void setOrientation(float orientation,
			      wmCallback *callback = NULL);

  /**
   * Start the NULL primitive on the robot
   */
  virtual void startNULL(float accel,
			 wmCallback *callback = NULL);
    
  /**
   * Set the robot's wheel velocities independently.
   * The NULL primitive should be running, or this will have no effect.
   */
  virtual void setWheels(float vl, float vr);
    
  /**
   * Stop the robot in its tracks and flush any queued behaviors.
   */
  virtual bool stop(void);

  /**
   * Internal callback method used to update the dashboard when a pivot/move
   * starts.
   */
  void motionStarted(acpObject *behavior);
    
  /**
   * Internal callback method used to update the dashboard when a pivot/move
   * finishes.
   *
   * @param status The status of the move.  (One of the aGARCIA_ERRFLAG
   * values.)
   * @param callback The callback to trigger.
   */
  void motionFinished(acpObject *behavior, int status, wmCallback *callback);

  /**
   * Check the object to make sure it is internally consistent.
   *
   * @return true
   */
  bool invariant(void)
  {
    assert(this->wm_dashboard != NULL);

    return true;
  };

 private:

  /**
   * The garcia whose wheels are being controlled.
   */
  acpGarcia &wm_garcia;

  /**
   * The last status from a pivot/move.
   */
  int wm_last_status;

  /**
   * Indicates that the robot is currently in motion.
   */
  bool wm_moving;

  float wm_speed;

  /**
   * Pointer to the dashboard that should be updated when doing pivots/moves.
   */
  dashboard *wm_dashboard;

  /**
   * The ledClient used to indicate motion.
   */
  ledClient wm_moving_notice;

  /**
   * The ledClient used to indicate an error while pivoting/moving.
   */
  ledClient wm_error_notice;
    
};

#endif
