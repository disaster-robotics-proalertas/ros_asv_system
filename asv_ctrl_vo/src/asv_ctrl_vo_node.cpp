#include "ros/ros.h"
#include <ros/console.h>
#include <vector>

#include "nav_msgs/OccupancyGrid.h"
#include "asv_msgs/StateArray.h"
#include "visualization_msgs/Marker.h"

#include "asv_ctrl_vo/asv_ctrl_vo.h"
#include "asv_ctrl_vo/asv_ctrl_vo_node.h"

int main(int argc, char *argv[])
{
  ros::init(argc, argv, "asv_ctrl_vo_node");
  ros::start();

  ROS_INFO("Starting VO-node!");

  ros::NodeHandle n;

  VelocityObstacleNode vo_node;
  VelocityObstacle *vo = new VelocityObstacle;

  ros::Publisher og_pub = n.advertise<nav_msgs::OccupancyGrid>("velocity_obstacle", 10);
  ros::Publisher mk_pub = n.advertise<visualization_msgs::Marker>("vo_markers", 1);
  ros::Publisher cmd_pub = n.advertise<geometry_msgs::Twist>("cmd_vel", 10);

  ros::Subscriber obstacle_sub = n.subscribe("obstacle_states",
                                             1,
                                             &VelocityObstacleNode::obstacleCallback,
                                             &vo_node);
  ros::Subscriber asv_sub = n.subscribe("asv/state",
                                        1,
                                        &VelocityObstacleNode::asvCallback,
                                        &vo_node);

  vo_node.initialize(&og_pub, &cmd_pub, &mk_pub, &obstacle_sub, &asv_sub, vo);
  vo_node.start();

  ros::shutdown();
  return 0;
}


VelocityObstacleNode::VelocityObstacleNode() : vo_(NULL),
                                               cmd_pub_(NULL),
                                               og_pub_(NULL),
                                               mk_pub_(NULL),
                                               obstacle_sub_(NULL),
                                               asv_sub_(NULL) {};

VelocityObstacleNode::~VelocityObstacleNode() {}

void VelocityObstacleNode::initialize(ros::Publisher *og_pub,
                                      ros::Publisher *cmd_pub,
                                      ros::Publisher *mk_pub,
                                      ros::Subscriber *obstacle_sub,
                                      ros::Subscriber *asv_sub,
                                      VelocityObstacle *vo)
{
  cmd_pub_ = cmd_pub;
  og_pub_ = og_pub;
  mk_pub_ = mk_pub;
  obstacle_sub_ = obstacle_sub;
  asv_sub_ = asv_sub;

  vo_ = vo;

  vo_->initialize(&obstacles_);

  initializeMarker();
}

void VelocityObstacleNode::start()
{
  ros::Rate loop_rate(10.0);

  while (ros::ok())
    {
      vo_->update(og_pub_);

      marker_.header.stamp = ros::Time();
      mk_pub_->publish(marker_);

      ros::spinOnce();
      loop_rate.sleep();
    }

}


void VelocityObstacleNode::asvCallback(const nav_msgs::Odometry::ConstPtr &msg)
{
  vo_->updateAsvState(msg);
}

void VelocityObstacleNode::obstacleCallback(const asv_msgs::StateArray::ConstPtr &msg)
{
  obstacles_ = msg->states;
}

void VelocityObstacleNode::clearMarkers()
{
}

void VelocityObstacleNode::initializeMarker()
{
  // Setup marker message
  marker_.header.stamp = ros::Time::now();
  marker_.header.frame_id = std::string("asv");
  marker_.ns = std::string("asv");
  marker_.type = visualization_msgs::Marker::POINTS;
  marker_.action = visualization_msgs::Marker::ADD;
  //marker_.pose = geometry_msgs::Pose();
  marker_.scale.x = 1.0;
  marker_.scale.y = 1.0;


  const int VEL_RES = vo_->getVelRes();
  const int ANG_RES = vo_->getAngRes();

  marker_.points.resize(vo_->getVelRes()*vo_->getAngRes());
  marker_.colors.resize(vo_->getVelRes()*vo_->getAngRes());

  vo_->initializeMarker(&marker_);
}
