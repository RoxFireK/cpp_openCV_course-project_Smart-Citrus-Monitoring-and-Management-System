#include<opencv2/imgcodecs.hpp>
#include<opencv2/highgui.hpp>
#include<opencv2/imgproc.hpp>
#include<iostream>
#include<string>
using namespace std;
using namespace cv;

Mat imgHSV,mask,imgResize;//H:色调，S：饱和度,V：明度
int hmin = 0,smin = 0,vmin = 0;
int hmax = 179,smax = 255,vmax = 255;
//查明颜色，可用于检查颜色的色调，饱和度，透明度，可用于上述因素的排序

int main(){

    string path = "C:/Users/Admin/Desktop/opencv/project/picture/test_pictures/bad_orange_test.webp";
    Mat img =imread(path);
    resize(img,imgResize,Size(),0.5,0.5);
    cvtColor(imgResize,imgHSV,COLOR_BGR2HSV);//将BGR颜色空间转换到HSV颜色空间(red)

    //生成窗口
    namedWindow("Trackbars",(640*200));
    //生成滑块调整选区大小，注意引用为地址，因为要改变原变量
    createTrackbar("Hue Min","Trackbars",&hmin,179);//Hue 色调 将色盘分为180度半圆，最大值为179
    createTrackbar("Hue Max","Trackbars",&hmax,179);
    createTrackbar("Sat Min","Trackbars",&smin,255);//Sat 饱和度
    createTrackbar("Sat Max","Trackbars",&smax,255);
    createTrackbar("Val Min","Trackbars",&vmin,255);//Val 明度
    createTrackbar("Val Max","Trackbars",&vmax,255);

    while(1){

        Scalar lower(hmin,smin,vmin);
        Scalar upper(hmax,smax,vmax);//设置上下阈值
        inRange(imgHSV,lower,upper,mask);
        
        imshow("Image",imgResize);
        imshow("Image HSV",imgHSV);
        imshow("Image Mask",mask);
        waitKey(1);
    }

return 0;
}

