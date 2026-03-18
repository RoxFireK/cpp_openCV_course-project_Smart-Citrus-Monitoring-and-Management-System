#include<opencv2/imgcodecs.hpp>
#include<opencv2/opencv.hpp>
#include<opencv2/highgui.hpp>
#include<opencv2/imgproc.hpp>
#include<iostream>
#include<windows.h>
#include<vector>
#include<string>

using namespace std;
using namespace cv;

int hmin = 0,smin =189 ,vmin = 116;//0 198 50
int hmax = 179,smax = 255,vmax = 255;
//已经在颜色探查程序中查找到的颜色

//用于处理图像进行形状匹配
Mat color_filter(Mat input_img){    
    Mat imgHSV,mask,imgblur;
    Mat imgCanny,imgDil;

    GaussianBlur(input_img,imgblur,Size(3,3),5,0);
    cvtColor(imgblur,imgHSV,COLOR_BGR2HSV);//将BGR颜色空间转换到HSV颜色空间(red)，以便于突出橘子形状

    // 颜色阈值，根据全局变量给出
    Scalar lower(hmin,smin,vmin);
    Scalar upper(hmax,smax,vmax);
    inRange(imgHSV,lower,upper,mask);

    // Canny边缘检测和膨胀
    GaussianBlur(mask,imgblur,Size(3,3),5,0);//高斯模糊去除噪点：自变量，因变量，模糊程度，模糊位置(x,y)
    Canny(imgblur,imgCanny,50,150);
    Mat kernel = getStructuringElement(MORPH_RECT,Size(7,7));//生成一个用于膨胀的核,参数只能为奇数!
    dilate(imgCanny,imgDil,kernel);//对形状线条进行加粗
    //imshow("img dil",imgDil);

    return imgDil;
}

//用于探测最外层橘子形状，由于大形状套小形状只会输出大形状，因此需要对橘子图像（最大）进行剔除
bool getContoursOrange(Mat imgDil,Mat img,Rect& orangeRect,Mat& orangeMask){
//此处orangeRect和orangeMask要传参，因为要修改原值
    vector<vector<Point>> Orangecontours;//二维数组储存橘子轮廓
    vector<Vec4i> hierarchy;//内置数组，数组元素包含四个point,表示层次结构

    findContours(imgDil,Orangecontours,hierarchy,RETR_EXTERNAL,CHAIN_APPROX_SIMPLE);//SIMPLE更节省内存
    //检查点
    if(Orangecontours.empty()){
        cout<<"No orange shape"<<endl;
        return false;
    }

    //最大形状为橘子
    double maxArea = 0;
    int maxi = -1;
    for(int i = 0;i<Orangecontours.size();i++){
        double areai = contourArea(Orangecontours[i]);
    if(areai>maxArea){
      maxArea = areai;
      maxi = i;
    }
  }
  //检查点
  if (maxi == -1) {
    cout << "No valid orange contour found!" << endl;
    return false;
  }

    vector<Point> Orangecontour = Orangecontours[maxi];
    double area = contourArea(Orangecontour);

    float peri = arcLength(Orangecontour,true);//生成周长
    vector<Point> conPoly;
    approxPolyDP(Orangecontour,conPoly,0.02*peri,true);
    //根据周长将复杂曲线分布转化为更简单的多边形，附shapes_detect_test.cpp

    string objectType;//判断是否为橘子
    if(area>500){ 
        int objCor = (int)conPoly.size();
        if(objCor > 4) {objectType="orange!";}//橘子形状简化后一般为多边形
        else{
        objectType="What is this??We need more orange!";
        return false;
        }
    }else{
        return false;
    }

    orangeRect = boundingRect(Orangecontour);//获取简化多边形的最小外接矩形
    rectangle(img,orangeRect.tl(),orangeRect.br(),Scalar(0,255,0),5);//注意此处要控制矩形边缘厚度
    putText(img,objectType,{orangeRect.x,orangeRect.y - 5},FONT_HERSHEY_PLAIN,2,Scalar(0,69,255),2);//放置标识文本      
    
    // 将 conPoly 放入一个临时的 vector<vector<Point>> 中进行绘制
    vector<vector<Point>> singleOrangeContour = {conPoly};
    drawContours(img,singleOrangeContour,0,Scalar(255,0,255),1);

    //创建橘子掩码，用于探测内部结构
    orangeMask = Mat::zeros(img.size(),CV_8UC1);
    drawContours(orangeMask,singleOrangeContour,0,Scalar(255),FILLED);

    return true;
}

int getContoursInner(Mat imgDil,Mat img,Rect orangeRect,Mat orangeMask){
    int badcount = 0;
    
    // 对整个 imgDil 应用整个 orangeMask，避免 ROI 裁剪
    Mat finalBinary;
    bitwise_and(imgDil, orangeMask, finalBinary); 
    
    vector<vector<Point>> contours;//和orange一样的步骤
    vector<Vec4i> hierarchy;
    findContours(finalBinary,contours,hierarchy,RETR_EXTERNAL,CHAIN_APPROX_SIMPLE);
    
    //遍历访问
    for (int i = 0;i<contours.size();i++){
        double area = contourArea(contours[i]);
        string objectType;

        if(area<=3000&&area>=50){ // 过滤小轮廓
        float peri = arcLength(contours[i],true);
        vector<Point> conPoly_i;
        approxPolyDP(contours[i],conPoly_i,0.02*peri,true);
        
        Rect boundRect_i = boundingRect(conPoly_i);

        // 判断检测图形形状
        int objCor = (int)conPoly_i.size();

        if(objCor >= 4) {//病斑一般为多边形
            if (area>0.65*(float)boundRect_i.width*(float)boundRect_i.height&&area>100){//避免边缘包容进去
            objectType = "bad";
            badcount++;
            }
        }

        // 绘制矩形 (坐标是绝对的)
        rectangle(img,boundRect_i.tl(),boundRect_i.br(),Scalar(0,255,0),5);
        putText(img,objectType,{boundRect_i.x,boundRect_i.y - 5},FONT_HERSHEY_PLAIN,2,Scalar(0,69,255),2);
        
        // 绘制轮廓 (使用临时 vector<vector<Point>>)
        vector<vector<Point>> singleContour = {conPoly_i}; 
        drawContours(img,singleContour,0,Scalar(255,0,255),1); 
        }
    }
        return badcount;//返回病斑数量
}

// 获取文件夹中所有图像文件
vector<string> getAllImageFiles(const string& folderPath) {
    vector<string> imageFiles;
    vector<string> imageExtensions = {".jpg", ".jpeg", ".png", ".bmp", ".tiff", ".tif", ".webp"};
    
    // 构建搜索路径
    string searchPath = folderPath + "\\*";
    
    WIN32_FIND_DATAA findFileData;
    HANDLE hFind = FindFirstFileA(searchPath.c_str(), &findFileData);
    
    if (hFind == INVALID_HANDLE_VALUE) {
        cout << "无法访问文件夹: " << folderPath << endl;
        return imageFiles;
    }
    
    do {
        // 跳过"."和".."目录
        if (strcmp(findFileData.cFileName, ".") == 0 || strcmp(findFileData.cFileName, "..") == 0) {
            continue;
        }
        
        // 检查是否是文件
        if (!(findFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
            string filename = findFileData.cFileName;
            string extension;
            
            // 获取文件扩展名
            size_t dotPos = filename.find_last_of(".");
            if (dotPos != string::npos) {
                extension = filename.substr(dotPos);
                
                // 转换为小写
                for (char& c : extension) {
                    c = tolower(c);
                }
                
                // 检查是否是支持的图像格式
                for (const string& imgExt : imageExtensions) {
                    if (extension == imgExt) {
                        string fullPath = folderPath + "\\" + filename;
                        imageFiles.push_back(fullPath);
                        break;
                    }
                }
            }
        }
    } while (FindNextFileA(hFind, &findFileData) != 0);
    
    FindClose(hFind);
    return imageFiles;
}

//以下是管理系统部分
class Orange {
public:
    int num;           // 橘子编号
    bool type;         // 橘子类型（好/坏）
    int badnum;        // 坏斑数量
    string imageName;  // 图像文件名

    Orange(int val, string name) : num(val), type(true), badnum(0), imageName(name) {}//构造函数
    ~Orange() {}//析构函数
    //功能函数定义
    int bad_oranges_num(vector<Orange> a);
    float bad_oranges_persent(vector<Orange> a);
    void bad_days(vector<Orange> a,vector<string> &bad_type);

};

//坏果个数统计
int Orange::bad_oranges_num(vector<Orange> a){
    int badOranges = 0;
    for(int i = 0;i<a.size();i++){
        if(!a[i].type){
            badOranges++;
        }
    }
return badOranges;
}

//坏果比例统计
float Orange::bad_oranges_persent(vector<Orange> a){
    float present = (float)bad_oranges_num(a)/a.size();
return present;
}

//根据霉斑数量，推断病坏程度
void Orange::bad_days(vector<Orange> a, vector<string> &bad_type){
    cout << "Local data shows:" << endl;
    
    // 清空并重新填充 bad_type 向量
    bad_type.clear();  // 清除原有内容
    
    for(int i = 0; i < a.size(); i++){
        if(!a[i].type){  // 只处理坏的橙子
            badnum = a[i].badnum;
            string severity;
            
            if(badnum < 3){
                severity = "slightly";
            }
            else if(badnum >= 3 && badnum < 6){
                severity = "moderately";
            }
            else if(badnum >= 6 && badnum < 10){
                severity = "extremely";
            }
            else{
                severity = "absolutely";
            }
            
            bad_type.push_back(severity); //vector数组使用 push_back 添加元素
            cout << "The " << i+1 << " orange is " << severity << " bad" << endl;
        }
    }
}
int main() {
    int choice;
    cout<<">>>>>>Welcome to the Smart Orchard Management System<<<<<<" << endl;
    cout<<">>>Press 1 to initialize orchard detection" << endl;
    cout<<">>>Press 2 to get the number of bad oranges" << endl;
    cout<<">>>Press 3 to get the proportion of bad oranges" << endl;
    cout<<">>>Press 4 to get the severity of oranges" << endl;
    cout<<">>>Press 5 to get information about a single orange" << endl;
    cout<<">>>Press 0 to exit"<<endl;
    cin >> choice;
    // 创建Orange对象数组
    vector<Orange> oranges;
    if(choice == 0){
        return 0;
    }
    if(choice == 1){
        string path = "C:/Users/Admin/Desktop/opencv/project/picture/orange_pictures";
        
        // 获取文件夹中所有图像文件
        vector<string> imageFiles = getAllImageFiles(path);
        
        if (imageFiles.empty()) {
            cout << "Image file not found in folder!" << endl;
            return -1;
        }
        
        cout << "find " << imageFiles.size() << " pictures" << endl;
        
        // 遍历并处理所有图像
        int imgIndex = 0;
        while (imgIndex < imageFiles.size()) {
            string imagePath = imageFiles[imgIndex];
            cout << "\n=== Processing the " << imgIndex + 1 << " picture ===" << endl;
            cout << "path: " << imagePath << endl;
            
            // 读取图像
            Mat img = imread(imagePath);
            
            if (img.empty()) {
                cout << "Unable to read image: " << imagePath << endl;
                imgIndex++;
                continue;
            }
            
            // 提取文件名
            size_t pos = imagePath.find_last_of("\\");
            string filename = (pos != string::npos) ? imagePath.substr(pos + 1) : imagePath;
            
            // 创建Orange对象
            Orange orange(imgIndex + 1, filename);
            
            // 处理图像
            Rect orangeRect;
            Mat imgDil, orangeMask;
            bool orangedetect;
            int badPiecesNumber = 0;
            
            imgDil = color_filter(img.clone());
            
            if (imgDil.empty() || imgDil.cols == 0 || imgDil.rows == 0) {
                cout << "Color filtering failed: " << imagePath << endl;
                orange.type = false;
                oranges.push_back(orange);
                imgIndex++;
                continue;
            }
            
            // 显示过滤后的图像
            /*imshow("Filtered Image", imgDil);
            waitKey(0);  // 短暂显示*/
            
            // 检测橘子
            orangedetect = getContoursOrange(imgDil, img, orangeRect, orangeMask);
            
            if(orangedetect){
                // 检测坏斑
                badPiecesNumber = getContoursInner(imgDil, img, orangeRect, orangeMask);
                cout << "Detected the number of bad spots: " << badPiecesNumber << endl;
                
                // 更新Orange对象
                orange.badnum = badPiecesNumber;
                orange.type = (badPiecesNumber == 0);  // 如果没有坏斑，橘子是好的
                
                // 显示结果
                imshow("Detection Result", img);
                waitKey(0);  // 短暂显示
            } else {
                cout << "Orange detection failed" << endl;
                orange.type = false;
            }
            
            // 将Orange对象添加到数组
            oranges.push_back(orange);
            
            // 等待用户按键继续下一个图像
            cout << "Press any key to continue processing the next image, or press' q 'to exit .." << endl;
            char key = waitKey(0);
            if (key == 'q' || key == 'Q') {
                cout << "User chooses to exit" << endl;
                break;
            }
            
            imgIndex++;
        }
        
        // 输出所有橘子的检测结果
        cout << "\n=== Summary of Test Results ===" << endl;
        cout << "We have dealt with it in total " << oranges.size() << " oranges" << endl;
        
        for (int i = 0; i < oranges.size(); i++) {
            cout << "orange " << oranges[i].num << " (" << oranges[i].imageName << "): ";
            if (oranges[i].type) {
                cout << "good orange";
                if (oranges[i].badnum > 0) {
                    cout << ",but " << oranges[i].badnum << " bads";
                }
            } else {
                cout << "bad orange";
                if (oranges[i].badnum > 0) {
                    cout << ",have " << oranges[i].badnum << " bads";
                }
            }
            cout << endl;
        }
        cout<<endl;
        
        destroyAllWindows();  // 关闭所有OpenCV窗口
        
    } else {
        cout << "exit" << endl;
    }
    vector<string> type;
    int bad_count = oranges[0].bad_oranges_num(oranges);
    float bad_persent = oranges[0].bad_oranges_persent(oranges);
    while(cin>>choice){
        switch (choice){
            case 2:
                {
                cout<<"The number of bad oranges is "<<bad_count<<'.'<<endl;
                break;
                }
            case 3:
                {
                cout<<"The persent of bad oranges is "<<bad_persent<<'.'<<endl;
                break;
                }
            case 4:
                {

                int severe_count = 0;
                oranges[0].bad_days(oranges,type);
                cout<<"Overall data shows:"<<endl;
                for(int i = 0;i<type.size();i++){
                    if(type[i]=="extremely"||type[i]=="absolutely"){
                        severe_count++;
                    }
                }
                float really_bad_per = (float)severe_count/bad_count;
                cout<<really_bad_per*100<<'%'<<" of the oranges were severely diseased."<<endl;
                break;
                }
            case 5:
                {
                int check_num;
                cout<<"Please enter the orange serial number you want to query"<<endl;
                cin>>check_num;
                cout<<"The "<<oranges[check_num-1].num<<" orange information:"<<endl;
                cout<<"filename:"<<oranges[check_num-1].imageName<<endl;
                if(!oranges[check_num-1].type){
                    cout<<"orange type: bad orange"<<endl;
                    cout<<"bad numbers: "<<oranges[check_num-1].badnum<<endl;
                    cout<<"bad level: "<<type[check_num-1]<<endl;
                }
                else{
                    cout<<"orange type: good orange"<<endl;
                }
                break;
                }
            case 0:
                return 0;
        }
    }
}
//orange 1单个测试
/*int main() {
    string path = "C:/Users/Admin/Desktop/opencv/project/picture/bad_orange.webp";
    Mat img =imread(path);
    Rect orangeRect;
    Mat imgDil,orangeMask;
    bool orangedetect;
    int badPiecesNumber;

        //检查点
        if (img.empty()) {
            cout << "ERROR: Could not read the image from path: " << path << endl;
            return -1;
        }

    imgDil = color_filter(img.clone()); // 使用 clone() 确保 imgDil 是一个独立的 Mat
    
        // 检查 imgDil 是否为空或尺寸为零
        if (imgDil.empty() || imgDil.cols == 0 || imgDil.rows == 0) {
            cout << "ERROR: color_filter returned an invalid image." << endl;
            return -1;
        }

    imshow("img Dil",imgDil);
    orangedetect = getContoursOrange(imgDil,img,orangeRect,orangeMask);
    
    if(orangedetect){
        imshow("image",img);
        waitKey(0); // 第一次暂停：显示外部轮廓
        badPiecesNumber = getContoursInner(imgDil,img,orangeRect,orangeMask);
        cout<<"Bad Pieces Found: "<<badPiecesNumber<<endl;
        imshow("image",img); // 修正：显示绘制了内部坏块的最终图像
        waitKey(0); // 第二次暂停：显示最终结果
    }
    else{
        cout<<"warning in detecting orange"<<endl;
    }
return 0;
}*/