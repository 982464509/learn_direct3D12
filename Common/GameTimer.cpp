//***************************************************************************************
// GameTimer.cpp by Frank Luna (C) 2011 All Rights Reserved.
//***************************************************************************************

#include <windows.h>
#include "GameTimer.h"

GameTimer::GameTimer() :
	mSecondsPerCount(0.0),
	mDeltaTime(-1.0),
	mBaseTime(0),
	mPausedTime(0),
	mPrevTime(0),
	mCurrTime(0),	
	mStopped(false)
{
	__int64 countsPerSec;	//性能计时器的频率（单位：计数/秒）
	QueryPerformanceFrequency((LARGE_INTEGER*)&countsPerSec);
	mSecondsPerCount = 1.0 / (double)countsPerSec;	
}

//返回自调用Reset函数开始不计暂停时间的总时间
float GameTimer::TotalTime()const
{	
	if( mStopped )
	{
		return (float)(((mStopTime - mPausedTime)-mBaseTime)*mSecondsPerCount);
	}	
	else
	{
		return (float)(((mCurrTime-mPausedTime)-mBaseTime)*mSecondsPerCount);
	}
}

float GameTimer::DeltaTime()const
{
	return (float)mDeltaTime;
}

void GameTimer::Reset()
{
	__int64 currTime;	
	QueryPerformanceCounter((LARGE_INTEGER*)&currTime);//性能计时器当前时刻值（以计数为单位）

	mBaseTime = currTime;
	mPrevTime = currTime;
	mStopTime = 0;
	mStopped  = false;
}

void GameTimer::Start()
{
	__int64 startTime;
	QueryPerformanceCounter((LARGE_INTEGER*)&startTime);

	// 累加调用stop和start这对方法之间的暂停时刻间隔
	//                     |<-------d------->|
	// ----*---------------*-----------------*------------> time
	//  mBaseTime       mStopTime        startTime     
	if( mStopped )
	{
		mPausedTime += (startTime - mStopTime);	// 累加暂停时间
		// 在重新开启计时器时，前一帧的时间mPrevTime是无效的，这是因为它存储的是暂停时前一
		// 帧的开始时刻，因此需要将它重置为当前时刻
		mPrevTime = startTime;
		// 已不再是停止状态……
		mStopTime = 0;
		mStopped  = false;
	}
}

void GameTimer::Stop()
{
	if( !mStopped )
	{
		__int64 currTime;
		QueryPerformanceCounter((LARGE_INTEGER*)&currTime);		
		mStopTime = currTime;
		mStopped  = true;
	}
}

void GameTimer::Tick()
{
	if( mStopped )
	{
		mDeltaTime = 0.0;
		return;
	}
	// 获得本帧开始显示的时刻
	__int64 currTime;
	QueryPerformanceCounter((LARGE_INTEGER*)&currTime);
	mCurrTime = currTime;	
	// 本帧与前一帧的时间差
	mDeltaTime = (mCurrTime - mPrevTime)*mSecondsPerCount;
	// 准备计算本帧与下一帧的时间差
	mPrevTime = mCurrTime;
	// 使时间差为非负值。DXSDK中的CDXUTTimer示例注释里提到：如果处理器处于节能模式，或者在
	// 计算两帧间时间差的过程中切换到另一个处理器时（即QueryPerformanceCounter函数的两次调
	// 用并非在同一处理器上），则mDeltaTime有可能会成为负值
	if(mDeltaTime < 0.0)
	{
		mDeltaTime = 0.0;
	}
}