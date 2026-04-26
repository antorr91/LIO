
#include "LioVideoApp.h"
#include <opencv2/opencv.hpp>
#include <iostream>

using namespace cv;

namespace{
void onMouse(int event,int x,int y,int,void* user)
{
 if(event!=EVENT_LBUTTONDOWN) return;
 auto* p=(Point*)user;
 p->x=x; p->y=y;
}
}

void runVideoApp(const std::string& videoPath)
{
 VideoCapture cap;
 bool hasVideo=false;

 if(!videoPath.empty())
 {
   cap.open(videoPath);
   if(cap.isOpened()) hasVideo=true;
 }

 Mat frame;
 Point click(-1,-1);

 namedWindow("LIO",WINDOW_NORMAL);
 resizeWindow("LIO",1280,800);
 setMouseCallback("LIO",onMouse,&click);

 bool playing=true;
 bool running=true;

 while(running)
 {
   if(hasVideo && playing)
   {
      cap>>frame;
      if(frame.empty())
      {
         cap.set(CAP_PROP_POS_FRAMES,0);
         cap>>frame;
      }
   }
   else
   {
      frame=Mat(
       800,1280,CV_8UC3,
       Scalar(245,244,240)
      );
   }

   rectangle(frame,Rect(20,20,1240,70),
      Scalar(225,225,225),FILLED);

   putText(frame,
      "LIO | Latent Interaction Observer",
      Point(50,65),
      FONT_HERSHEY_SIMPLEX,
      0.9,
      Scalar(60,30,60),
      2);

   rectangle(frame,Rect(50,120,840,500),
      Scalar(150,150,150),2);

   putText(frame,
      "Video / Tracking View",
      Point(90,160),
      FONT_HERSHEY_SIMPLEX,
      0.8,
      Scalar(70,30,60),
      2);

   rectangle(frame,Rect(930,120,280,500),
      Scalar(150,150,150),2);

   const char* labels[]={
      "Approach",
      "Contact",
      "Freezing",
      "Grooming"
   };

   for(int i=0;i<4;i++)
   {
      rectangle(frame,
       Rect(970,210+i*80,200,50),
       Scalar(120,40,70),
       FILLED);

      putText(frame,
        labels[i],
        Point(1010,243+i*80),
        FONT_HERSHEY_SIMPLEX,
        0.6,
        Scalar(255,255,255),
        2);
   }

   rectangle(frame,Rect(40,660,1180,90),
      Scalar(230,230,230),FILLED);

   putText(frame,
     "SPACE play/pause | click annotate | Q quit",
     Point(90,715),
     FONT_HERSHEY_SIMPLEX,
     0.7,
     Scalar(40,40,40),
     2);

   if(click.x>=0)
      circle(frame,click,8,Scalar(0,0,180),FILLED);

   imshow("LIO",frame);

   int k=waitKey(30);

   if(k==' ') playing=!playing;
   if(k=='q' || k==27) running=false;
 }

 destroyAllWindows();
}
