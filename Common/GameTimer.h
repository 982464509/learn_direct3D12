//***************************************************************************************
// GameTimer.h by Frank Luna (C) 2011 All Rights Reserved.
//***************************************************************************************

#ifndef GAMETIMER_H	//防御式编程
#define GAMETIMER_H

class GameTimer
{
public:
	GameTimer();

	float TotalTime()const; // in seconds
	float DeltaTime()const; // in seconds

	// 在开始消息循环之前调用
	void Reset(); 
	// 解除计时器暂停时调用
	void Start(); 
	// 暂停计时器时调用
	void Stop();  
	// 每帧都要调用
	void Tick();  

private:
	//秒/计数
	double mSecondsPerCount;
	//两帧之间的时间间隔
	double mDeltaTime;
	//应用程序的开始时刻
	__int64 mBaseTime;
	//存储的是所有暂停时间之和
	__int64 mPausedTime;
	//计时器停止（暂停）的时刻，借此即可记录暂停的时间
	__int64 mStopTime;

	__int64 mPrevTime;

	__int64 mCurrTime;

	bool mStopped;
};

#endif