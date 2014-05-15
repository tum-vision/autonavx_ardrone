#!/usr/bin/env python
import rospy
import roslib; roslib.load_manifest('ardrone_python')
from ardrone_autonomy.msg import Navdata
from std_msgs.msg import Empty
from geometry_msgs.msg import Twist, Vector3

def callback(navdata):
    t = navdata.header.stamp.to_sec()
    print("received odometry message: time=%f battery=%f vx=%f vy=%f z=%f yaw=%f"%(t,navdata.batteryPercent,navdata.vx,navdata.vy,navdata.altd,navdata.rotZ))

if __name__ == '__main__':
    rospy.init_node('example_node', anonymous=True)
    
    # subscribe to navdata (receive from quadrotor)
    rospy.Subscriber("/ardrone/navdata", Navdata, callback)
    
    # publish commands (send to quadrotor)
    pub_velocity = rospy.Publisher('/cmd_vel', Twist)
    pub_takeoff = rospy.Publisher('/ardrone/takeoff', Empty)
    pub_land = rospy.Publisher('/ardrone/land', Empty)
    pub_reset = rospy.Publisher('/ardrone/reset', Empty)
    
    print("ready!")
    rospy.sleep(1.0)
    
    print("takeoff..")
    pub_takeoff.publish(Empty())
    rospy.sleep(5.0)
    
    while not rospy.is_shutdown():
      # fly forward..
      pub_velocity.publish(Twist(Vector3(0.1,0,0),Vector3(0,0,0)))
      rospy.sleep(2.0)
      
      # fly backward..
      pub_velocity.publish(Twist(Vector3(-0.1,0,0),Vector3(0,0,0)))
      rospy.sleep(2.0)
    
