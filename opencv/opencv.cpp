//--------------------------------------【程序说明】----------------------------------------------
//		程序描述：配合opencv 实现微信自动跳一跳
//		开发测试所用操作系统： Windows 10 64bit
//		开发测试所用IDE版本： Visual Studio 2017
//		开发测试所用OpenCV版本：3.4.0 
//		开发时间：2018年2月15日07:58:32
//		版本：2.x
//		注意事项：只能生成Release版本，不能生成Debug


/*
关于程序改善的说明：V1.4
时间：2018年2月19日16:39:34
1-增加实现检测游戏结束画面，实行自动结束游戏
时间：2018年2月22日21:23:41
2-改进检测方块中心心得算法采用从右侧向左扫描
3-对代码结构进行调整
时间：2018年2月23日08:43:11
保存临死时的状态，便于分析为啥会死掉
下一步：增加检测死之前的3幅画面，分析挂掉原因
时间：2018年3月21日23:10:06
在检测中心的函数中增加判断小人的位置函数，遍历像素的时候，要根据小人的位置选择要从屏幕的左侧还是右侧进行遍历。效果可能会好一点
*/

#include <iostream>
#include <stdio.h>
#include "stdafx.h"
#include<opencv2/opencv.hpp>

#pragma warning(disable:4996)//不添加此句，将会产生以下错误
//error C4996 : 'sprintf' : This function or variable may be unsafe.Consider using sprintf_s instead.To disable deprecation, use _CRT_SECURE_NO_WARNINGS.See online help for details.

using namespace cv;
using namespace std;


void ShowHelpText(void);
void ScreenShot(void);
int GetDistance(Point firstpos, Point secondpos);
void Press(int distance);
bool MatchOver(Mat Shoot, Mat Over);
bool GetCirclePoint(Mat ImageRGB, Mat Circle, Point &circlepos);
void GetPeoplePoint(Mat ImageRGB, Mat People, Point &peoplepos);
void GetCenter(Mat ImageRGB, Point peoplepos, Point &centerpos);


/*将DEBUG改成1，将会成佛*/
#define DEBUG   1
/*
* @function main
*/
int main(void)
{
	ShowHelpText();
  
	/*1-读取用于模板匹配的图片*/
	Mat ImagePeople = imread("ImagePeople.jpg");
	Mat ImageOver = imread("ImageOver.jpg");
	Mat ImageCircle = imread("ImageCircle.jpg");

	while (1)
	{
		_sleep(1000);

		/*2-截图并读取图片分析*/
		ScreenShot();
		Mat ImageShoot = imread("screenshot.png");
		Mat ImageSrc = ImageShoot.clone();

		/*3-如果当前图片和游戏结束画面匹配成功，则退出循环结束游戏*/
		if (MatchOver(ImageShoot, ImageOver))
		{
			cout << "Game Over!!!" << endl;
			break;
		}

		/*4-模板匹配，获取小人所在位置*/
		Point peoplepos;
		GetPeoplePoint(ImageShoot, ImagePeople,peoplepos);
		cout << "小人坐标X:" << peoplepos.x << "  Y:" << peoplepos.y << endl;
		circle(ImageSrc,
			Point(peoplepos.x, peoplepos.y),
			10,
			Scalar(0, 255, 0),
			2,
			8,
			0);
		putText(ImageSrc, "greenpeople", peoplepos, FONT_HERSHEY_COMPLEX, 2, Scalar(0, 255, 0));


		int distance;
		/*5-模板匹配，如果获取到了小圆点，则使用小圆点计算距离*/
		Point circlepos;
		if (GetCirclePoint(ImageShoot, ImageCircle,circlepos) == true)
		{
			cout << "find circle point" << endl;
			distance = GetDistance(circlepos, peoplepos);
			cout << "圆圈坐标X:" << circlepos.x << "  Y:" << circlepos.y << endl;
			circle(ImageSrc,
				Point(circlepos.x, circlepos.y),
				10,
				Scalar(0, 0, 255),
				2,
				8,
				0);
		putText(ImageSrc, "redcircle", circlepos, FONT_HERSHEY_COMPLEX,2, Scalar(0, 0, 255));
		}

		/*6-如果没有获取到小圆点，用下一物体中心进行计算距离*/
		else
		{
			Point centerpos;
			GetCenter(ImageShoot, peoplepos, centerpos);
			cout << "物体中心X:" << centerpos.x << "  Y:" << centerpos.y << endl;
			distance = GetDistance(centerpos, peoplepos);

			circle(ImageSrc,
				Point(centerpos.x, centerpos.y),
				10,
				Scalar(255, 0, 0),
				2,
				8,
				0);
			putText(ImageSrc, "bluecenter", centerpos, FONT_HERSHEY_COMPLEX, 2, Scalar(255, 0, 0) );
		}

		/*7-根据计算的距离，计算时间，进行模拟按压操作*/
		Press(distance);

		/*8-保存最后图片，分析挂掉原因*/
		imwrite("last.png", ImageSrc);
	}
	waitKey(0);
	system("pause");
	return 0;
}

/// Function definitions

/*计算要跳的格的中心位置，传入小人所在位置*/
/*还是用直接从上到下进行检测，效果可能好一点*/
void GetCenter(Mat ImageRGB, Point peoplepos,Point &centerpos)
{
	/*1-转换为灰度图*/
	Mat ImageGray;
	cvtColor(ImageRGB, ImageGray, CV_RGB2GRAY);

	/*2-高斯模糊*/
	Mat dst;
	GaussianBlur(ImageGray, dst, Size(5, 5), 0);
	
	/*3-边缘检测*/
	Mat canny_img;
	Canny(dst, canny_img, 1, 10);
#if DEBUG
	namedWindow("canny图像输出调试", WINDOW_GUI_EXPANDED);
	imshow("canny图像输出调试", canny_img);

	for (int y = (peoplepos.y - 175); y < (peoplepos.y + 190); y++)
	{
		for (int x = (peoplepos.x - 38); x < (peoplepos.x + 38); x++)
		{
			canny_img.at<uchar>(y, x) = 0;
		}
	}
#endif 
	/*4-从上到下，从右向左扫描，找到要跳物体的最上面的点的坐标，也是物体中心得x坐标*/
	int flag = 0;
	for (int y = int(canny_img.rows*0.2); y<int(canny_img.rows*0.6); y++)
	{
		/*如果找到了就及时退出循环*/
		if (flag)
			break;
	//	uchar *col = canny_img.ptr<uchar>(j);
		for (int x = canny_img.cols-100; x > 100; x--)
		{
			int  k = canny_img.at<uchar>(y, x) ;
			if (k == 255)
			{
				centerpos.x = x;
				centerpos.y = y;
				flag = 1;
				break;
			}
		}
	}
	

	Scalar r = ImageRGB.at<Vec3b>(centerpos.y+3, centerpos.x);

	int tempy = 0;
	Mat ImageColor;
	/*5-如果小人在左边，从物体最上方的点开始向下扫描，从右到左扫，找到物体的最右侧的点的坐标，根据彩色图RGB三个分量相等去找*/
	if (peoplepos.x <= (ImageRGB.cols / 2))
	{
		cout << "小人在左侧" << endl;
		int tempx = 0;
		for (int y = centerpos.y; y < (centerpos.y + 300); y++)
		{
			for (int x = ImageRGB.cols; x > centerpos.x; x--)
			{
				Scalar k = ImageRGB.at<Vec3b>(y, x);
				if (k == r)
				{
					if (x > tempx)
					{/*把做标记录下来*/
						tempx = x;
						tempy = y;
					}
					break;
				}
			}
		}
	}

	/*5-如果小人在右边，从物体最上方的点开始向下扫描，从左向右扫，找到物体的最左侧的点的坐标，根据彩色图RGB三个分量相等去找*/
	if (peoplepos.x > (ImageRGB.cols / 2))
	{
		cout << "小人在右侧" << endl;
		int tempx = centerpos.x;
		for (int y = centerpos.y; y < (centerpos.y + 300); y++)
		{
			for (int x = 0; x < centerpos.x; x++)
			{
				Scalar k = ImageRGB.at<Vec3b>(y, x);
				if (k == r)
				{
					if (x < tempx)
					{/*把做标记录下来*/
						tempx = x;
						tempy = y;
					}
					break;
				}
			}
		}
	}

	centerpos.y = tempy;

#if DEBUG	
	cvtColor(dst, ImageColor, CV_GRAY2BGR);
	circle(ImageColor,
		Point(centerpos.x, centerpos.y),
		10,
		Scalar(0, 0, 255),
		2,
		8,
		0);
	circle(ImageColor,
		Point(centerpos.x, centerpos.y),
		10,
		Scalar(0, 0, 255),
		2,
		8,
		0);
	namedWindow("找中心图像输出调试", WINDOW_GUI_EXPANDED);
	imshow("找中心图像输出调试", ImageColor);
#endif



}

/*匹配小人，并返回小人的坐标*/
void GetPeoplePoint(Mat ImageRGB, Mat People , Point &peoplepos)
{
	/*1-读取截图和圆圈的图片，并转换为灰度图*/
	Mat ImageGray, ImagePeople;

	cvtColor(ImageRGB, ImageGray, CV_RGB2GRAY);
	cvtColor(People, ImagePeople, CV_RGB2GRAY);

	/*2-模板匹配*/
	Mat ImageMatched;  
	matchTemplate(ImageGray, ImagePeople, ImageMatched, TM_CCOEFF_NORMED);

	/*3-找到最佳匹配位置*/
	double minVal, maxVal;
	Point minLoc, maxLoc;
	minMaxLoc(ImageMatched, &minVal, &maxVal, &minLoc, &maxLoc);
	
	/*4-输出调试一下*/
#if DEBUG
	//画个圈圈，标记一下
	circle(ImageRGB,
		Point(maxLoc.x + ImagePeople.cols / 2, maxLoc.y + ImagePeople.rows / 2 + 70),
		10,
		Scalar(0, 0, 255),
		2,
		8,
		0);
	namedWindow("小人匹配图像输出调试", WINDOW_GUI_EXPANDED);
	imshow("小人匹配图像输出调试", ImageRGB);
#endif
	peoplepos.x = maxLoc.x + ImagePeople.cols / 2;
	peoplepos.y = maxLoc.y + ImagePeople.rows / 2 +70;



}

/*匹配圆点，并返回圆点坐标*/
bool GetCirclePoint(Mat ImageRGB, Mat Circle, Point &circlepos)
{
	/*1-读取截图和圆圈的图片，并转换为灰度图*/
	Mat ImageGray, ImageCircle;

	cvtColor(ImageRGB, ImageGray, CV_RGB2GRAY);
	cvtColor(Circle, ImageCircle, CV_RGB2GRAY);

	/*2-模板匹配*/
	Mat ImageMatched;
	matchTemplate(ImageGray, ImageCircle, ImageMatched, TM_CCOEFF_NORMED);

	/*3-找到最佳匹配位置*/
	double minVal, maxVal;
	Point minLoc, maxLoc;
	minMaxLoc(ImageMatched, &minVal, &maxVal, &minLoc, &maxLoc);

	/*只有匹配程度达到了0.9，才认为有圆点出现，否则不返回圆点坐标*/
	if (maxVal > 0.9)
	{
		/*4-输出调试一下*/
#if DEBUG
		//画个圈圈，标记一下
		circle(ImageRGB,
			Point(maxLoc.x + ImageCircle.cols / 2, maxLoc.y + ImageCircle.rows / 2),
			10,
			Scalar(0, 0, 255),
			2,
			8,
			0);
		namedWindow("圆圈匹配图像输出调试", WINDOW_GUI_EXPANDED);
		imshow("圆圈匹配图像输出调试", ImageRGB);
#endif
		circlepos.x = maxLoc.x + ImageCircle.cols / 2;
		circlepos.y = maxLoc.y + ImageCircle.rows / 2;
		return true;
	}

	else
		return false;

}

/*匹配游戏是不是结束，结束返回true*/
bool MatchOver(Mat ImageRGB,Mat Over)
{
	/*1-转灰度图*/
	Mat ImageGray, ImageOver;
	cvtColor(ImageRGB, ImageGray, CV_RGB2GRAY);
	cvtColor(Over, ImageOver, CV_RGB2GRAY);

	/*2-模板匹配*/
	Mat ImageMatched;
	matchTemplate(ImageGray, ImageOver, ImageMatched, TM_CCOEFF_NORMED);

	/*3-找到最佳匹配位置*/
	double minVal, maxVal;
	Point minLoc, maxLoc;
	minMaxLoc(ImageMatched, &minVal, &maxVal, &minLoc, &maxLoc);

	/*只有匹配程度达到了0.9，才认为有圆点出现，否则不返回圆点坐标*/
	if (maxVal > 0.9)
		return true;
	else
		return false;
}

/*截图并保存*/
void ScreenShot(void)
{
	/*使用 adb 命令进行截图,并保存到内存卡中*/
	system("adb shell screencap -p /sdcard/screenshot.png");
	/*将截图放到工程目录文件夹下，文件名screenshot.png*/
	system("adb pull /sdcard/screenshot.png");
}

/*通过adb命令按压屏幕，传入要按压的距离*/
void Press(int distance)
{
	char str[50];
	int time = (int)(distance * 1.385);
	/*加上随机数使得每次按压都是在点（300,400）-（400,500）之间*/
	int rand_x = int(300 + rand() % 100);
	int rand_y = int(400 + rand() % 100);

	/*格式化字符串
	第一个第二个参数：按压点的位置
	第三个第四个参数：释放点的位置
	第五个参数：按压的时间*/
	sprintf(str, "adb shell input swipe %d %d %d %d %d", rand_x, rand_y, rand_x, rand_y, time);
	/*命令输出*/
	system(str);
}

/*计算距离两个点之间的距离*/
int GetDistance(Point firstpos, Point secondpos)
{
	int x = firstpos.x - secondpos.x;
	int y = firstpos.y - secondpos.y;
	return (int)(pow((pow(x, 2) + pow(y, 2)), 0.5));
}

/*输出一些帮助信息*/
static void ShowHelpText(void)
{
	//输出一些帮助信息  
	printf("\n\t\t这是使用opencv实现自动玩微信跳一跳小程序\n\n");
	printf("\n\t\t打开手机的调试功能，并打开微信跳一跳界面\n\n");
	printf("\t\t键盘按键【ESC】- 退出程序\n\n");
}