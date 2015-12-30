#ifndef _DATACOLLECTION_H
#define _DATACOLLECTION_H

#include "general.h"

#define UNDEFINED_VALUE -99999

class IDataCollector
{
public:
	virtual float Collect();
	virtual void Init();
};

class MeasureMeta
{
public:
	int Id;
	bool canRaiseAlarm;
	float latestValue;
	String format;
	int iconHeight; int iconWidth; const unsigned int * iconData;

	//
	// some stats
	float dayMin;
	float dayMax;
	float progressBarMin; float progressBarMax;
	unsigned long timeOfSampling;

	MeasureZone latestLevel;

	//
	// Constructor
	MeasureMeta(int id,  MeasureCheck * warning,  MeasureCheck * error, IDataCollector * dataCollector, String fmt) :
		Id(id), DataCollector(dataCollector), format(fmt)
	{
		WarningCheck = warning;
		ErrorCheck = error;
		canRaiseAlarm = true;
	}

	//
	// Constructor - no measure checks will be performed
	MeasureMeta(int id, IDataCollector * dataCollector, String fmt) : Id(id), DataCollector(dataCollector), format(fmt)
	{
		canRaiseAlarm = true;
		WarningCheck = new NoMeasureCheck();
		ErrorCheck = new NoMeasureCheck();
	}

	void Init()
	{
		latestValue = dayMin = dayMax = UNDEFINED_VALUE;
		timeOfSampling = 0;
		latestLevel = MeasureZone_Normal;
		DataCollector->Init();
	}

	//
	// Gets the latest value and update level
	void Update()
	{
		latestValue = DataCollector->Collect();
		latestLevel = CheckAgainstLevels();

		unsigned long t = Time.now();

		// update stats
		bool isSameDayThanLastSampling = Time.day(t) == Time.day(timeOfSampling);
		if (isSameDayThanLastSampling && !(timeOfSampling == 0))
		{
			dayMin = latestValue < dayMin ? latestValue : dayMin;
			dayMax = latestValue > dayMax ? latestValue : dayMax;
		}
		else
		{
			dayMin = dayMax = latestValue;
		}
		
		timeOfSampling = t;

		Particle.publish(String::format("events.measures.%d.capture", Id), String::format("%f", latestValue));
	}

private:
	MeasureCheck * WarningCheck;
	MeasureCheck * ErrorCheck;
	IDataCollector * DataCollector;

	//
	// Checks the latest value against the defined zones
	MeasureZone CheckAgainstLevels()
	{
		if (!ErrorCheck->Test(latestValue))
			return MeasureZone_Critical;

		if (!WarningCheck->Test(latestValue))
			return MeasureZone_Warning;

		return MeasureZone_Normal;
	}
};

class DataCollectorManager {
public:
	DataCollectorManager(int8_t collectionIndicatorPin);
	void Init(MeasureMeta ** measures);
	void Collect(void(*onMeasureCollectionDone)(MeasureMeta *measure));
	int collectionInterval;

private:
	MeasureMeta ** _collectors;
	int8_t _collectionIndicatorPin;

	static bool isCurrentlyCollecting;
};

#endif