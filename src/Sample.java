
/******************************************************************************\
* Copyright (C) 2012-2013 Leap Motion, Inc. All rights reserved.               *
* Leap Motion proprietary and confidential. Not for distribution.              *
* Use subject to the terms of the Leap Motion SDK Agreement available at       *
* https://developer.leapmotion.com/sdk_agreement, or another agreement         *
* between Leap Motion and you, your company or other organization.             *
\******************************************************************************/
 
import java.io.IOException;
import java.lang.Math;
import com.leapmotion.leap.*;
import com.leapmotion.leap.Gesture.State;
import java.io.IOException;
import java.io.PrintWriter;
import java.net.ServerSocket;
import java.net.Socket;
import java.util.Date;
import org.json.simple.JSONObject;
import java.io.StringWriter;
import java.io.FileReader;
import java.io.FileWriter;

class SampleListener extends Listener {


    /**
     * A TCP server that runs on port 9090.  When a client connects, it
     * sends the client the current date and time, then closes the
     * connection with that client.  Arguably just about the simplest
     * server you can write.
     */
    // public class DateServer {

    //     /**
    //      * Runs the server.
    //      */
    //     public static void main(String[] args) throws IOException {
    //         ServerSocket listener = new ServerSocket(9090);
    //         try {
    //             while (true) {
    //                 Socket socket = listener.accept();
    //                 try {
    //                     PrintWriter out =
    //                         new PrintWriter(socket.getOutputStream(), true);
    //                     out.println(new Date().toString());
    //                 } finally {
    //                     socket.close();
    //                 }
    //             }
    //         }
    //         finally {
    //             listener.close();
    //         }
    //     }
    // }

    public void onInit(Controller controller) {
        System.out.println("Initialized");
    }
 
    public void onConnect(Controller controller) {
        System.out.println("Connected");
        controller.enableGesture(Gesture.Type.TYPE_SWIPE);
        controller.enableGesture(Gesture.Type.TYPE_CIRCLE);
        controller.enableGesture(Gesture.Type.TYPE_SCREEN_TAP);
        controller.enableGesture(Gesture.Type.TYPE_KEY_TAP);
    }
 
    public void onDisconnect(Controller controller) {
        //Note: not dispatched when running in a debugger.
        System.out.println("Disconnected");
    }
 
    public void onExit(Controller controller) {
        System.out.println("Exited");
    }
 
    public boolean onLeft(Vector normalizedPosition) {
        // takes normalized hand position vector, returns whether hand is on left side of interactionBox
        return normalizedPosition.getX() < 0.25;
    }
 
    public boolean onRight(Vector normalizedPosition) {
        return normalizedPosition.getX() > 0.75;
    }
 
    public boolean onTop(Vector normalizedPosition) {
        return normalizedPosition.getY() > 0.75;
    }
 
    public boolean onBottom(Vector normalizedPosition) {
        return normalizedPosition.getY() < 0.25;
    }
 

 
 
    public void onFrame(Controller controller) {
        // Get the most recent frame and report some basic information
        Frame frame = controller.frame();
        if (frame.timestamp() % 100 == 0) {
            // System.out.println("Frame id: " + frame.id()
            //         + ", timestamp: " + frame.timestamp()
            //         + ", hands: " + frame.hands().count()
            //         + ", fingers: " + frame.fingers().count()
            //         + ", tools: " + frame.tools().count()
            //         + ", gestures " + frame.gestures().count());
 
            InteractionBox box = frame.interactionBox();
 
 
            //Get hands
            for (Hand hand : frame.hands()) {
                String handType = hand.isLeft() ? "Left hand" : "Right hand";
                Vector normPos = box.normalizePoint(hand.palmPosition());
                // System.out.println("  " + handType + ", id: " + hand.id()
                //         + ", normalized palm position: " + normPos);
 
                // System.out.println("LEFT: " + onLeft(normPos) + " " + normPos.getX());
                // System.out.println("RIGHT: " + onRight(normPos) + " " + normPos.getX());
                // System.out.println("TOP: " + onTop(normPos));
                // System.out.println("BOTTOM: " + onBottom(normPos));
 
                // Get the hand's normal vector and direction
                Vector normal = hand.palmNormal();
                Vector direction = hand.direction();
 
                // Calculate the hand's pitch, roll, and yaw angles
                // System.out.println("  pitch: " + Math.toDegrees(direction.pitch()) + " degrees, "
                //         + "roll: " + Math.toDegrees(normal.roll()) + " degrees, "
                //         + "yaw: " + Math.toDegrees(direction.yaw()) + " degrees");
 
                try{
                    JSONObject obj = new JSONObject();

                    obj.put("timestamp", frame.timestamp());
                    obj.put("fingers", frame.fingers().count());
                    float[] positions = new float[3];
                    positions[0] = normPos.get(0);
                    positions[1] = normPos.get(1);
                    positions[2] =  normPos.get(2);
                    obj.put("norm_position_x", positions[0]);
                    obj.put("norm_position_y", positions[1]);
                    obj.put("norm_position_z", positions[2]);

                    // StringWriter out = new StringWriter();
                    // obj.writeJSONString(out);
                      
                    // String jsonText = out.toString();
                    // System.out.print(jsonText);

                    System.out.println("Writting JSON into file ...");
                    System.out.println(obj); 
                    FileWriter jsonFileWriter = new FileWriter("leap_data.json"); 
                    jsonFileWriter.write(obj.toJSONString()); 
                    jsonFileWriter.flush(); 
                    jsonFileWriter.close(); 
                    System.out.println("Done");
              }
              catch (Exception e){
                e.printStackTrace();
              }

             // try { 
             //    System.out.println("Writting JSON into file ...");
             //    System.out.println(obj); 
             //    FileWriter jsonFileWriter = new FileWriter(file); 
             //    jsonFileWriter.write(obj.toJSONString()); 
             //    jsonFileWriter.flush(); 
             //    jsonFileWriter.close(); 
             //    System.out.println("Done");
             //  } catch (IOException e) { e.printStackTrace(); }
              
                /*
                // Get arm bone
                Arm arm = hand.arm();
                System.out.println("  Arm direction: " + arm.direction()
                        + ", wrist position: " + arm.wristPosition()
                        + ", elbow position: " + arm.elbowPosition());
 
                // Get fingers
                for (Finger finger : hand.fingers()) {
                    System.out.println("    " + finger.type() + ", id: " + finger.id()
                            + ", length: " + finger.length()
                            + "mm, width: " + finger.width() + "mm");
 
                    //Get Bones
                    for (Bone.Type boneType : Bone.Type.values()) {
                        Bone bone = finger.bone(boneType);
                        System.out.println("      " + bone.type()
                                + " bone, start: " + bone.prevJoint()
                                + ", end: " + bone.nextJoint()
                                + ", direction: " + bone.direction());
                    }
                }
                */
            }
 
            // Get tools
            for (Tool tool : frame.tools()) {
                System.out.println("  Tool id: " + tool.id()
                        + ", position: " + tool.tipPosition()
                        + ", direction: " + tool.direction());
            }
 
            GestureList gestures = frame.gestures();
            for (int i = 0; i < gestures.count(); i++) {
                Gesture gesture = gestures.get(i);
                /*
                switch (gesture.type()) {
                    case TYPE_CIRCLE:
                        CircleGesture circle = new CircleGesture(gesture);
 
                        // Calculate clock direction using the angle between circle normal and pointable
                        String clockwiseness;
                        if (circle.pointable().direction().angleTo(circle.normal()) <= Math.PI / 2) {
                            // Clockwise if angle is less than 90 degrees
                            clockwiseness = "clockwise";
                        } else {
                            clockwiseness = "counterclockwise";
                        }
 
                        // Calculate angle swept since last frame
                        double sweptAngle = 0;
                        if (circle.state() != State.STATE_START) {
                            CircleGesture previousUpdate = new CircleGesture(controller.frame(1).gesture(circle.id()));
                            sweptAngle = (circle.progress() - previousUpdate.progress()) * 2 * Math.PI;
                        }
 
                        System.out.println("  Circle id: " + circle.id()
                                + ", " + circle.state()
                                + ", progress: " + circle.progress()
                                + ", radius: " + circle.radius()
                                + ", angle: " + Math.toDegrees(sweptAngle)
                                + ", " + clockwiseness);
                        break;
                    case TYPE_SWIPE:
                        SwipeGesture swipe = new SwipeGesture(gesture);
                        System.out.println("  Swipe id: " + swipe.id()
                                + ", " + swipe.state()
                                + ", position: " + swipe.position()
                                + ", direction: " + swipe.direction()
                                + ", speed: " + swipe.speed());
                        break;
                    case TYPE_SCREEN_TAP:
                        ScreenTapGesture screenTap = new ScreenTapGesture(gesture);
                        System.out.println("  Screen Tap id: " + screenTap.id()
                                + ", " + screenTap.state()
                                + ", position: " + screenTap.position()
                                + ", direction: " + screenTap.direction());
                        break;
                    case TYPE_KEY_TAP:
                        KeyTapGesture keyTap = new KeyTapGesture(gesture);
                        System.out.println("  Key Tap id: " + keyTap.id()
                                + ", " + keyTap.state()
                                + ", position: " + keyTap.position()
                                + ", direction: " + keyTap.direction());
                        break;
                    default:
                        System.out.println("Unknown gesture type.");
                        break;
                }
                */
            }
 
 
            if (!frame.hands().isEmpty() || !gestures.isEmpty()) {
                System.out.println();
            }
        }
    }
}
 
class Sample {
    public static void main(String[] args) {
        // Create a sample listener and controller
        SampleListener listener = new SampleListener();
        Controller controller = new Controller();
 
        // Have the sample listener receive events from the controller
        controller.addListener(listener);
 
        // Keep this process running until Enter is pressed
        System.out.println("Press Enter to quit...");
        try {
            System.in.read();
        } catch (IOException e) {
            e.printStackTrace();
        }
 
        // Remove the sample listener when done
        controller.removeListener(listener);
    }
}