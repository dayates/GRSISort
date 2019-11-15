#ifndef TDATAPARSER_H
#define TDATAPARSER_H

/** \addtogroup Sorting
 *  @{
 */

/////////////////////////////////////////////////////////////////
///
/// \class TDataParser
///
/// The TDataParser is the DAQ dependent part of GRSISort.
/// It takes raw data and converts it into a generic TFragment
/// that the rest of GRSISort can deal with.
/// This is where event word masks are applied, and any changes
/// to the event format must be implemented.
///
/////////////////////////////////////////////////////////////////

#include "Globals.h"
#include <ctime>
#include <sstream>
#include <vector>
#include <map>
#include <limits>

#ifndef __CINT__
#include <memory>
#endif

#include "TChannel.h"
#include "TFragment.h"
#include "TPPG.h"
#include "TScaler.h"
#include "TFragmentMap.h"
#include "ThreadsafeQueue.h"
#include "TEpicsFrag.h"
#include "TGRSIOptions.h"

class TRawEvent;

class TDataParser {
public:
   TDataParser();
   virtual ~TDataParser();
   virtual void SetNoWaveForms(bool temp = true) { fNoWaveforms = temp; }
   virtual void SetRecordDiag(bool temp = true) { fRecordDiag = temp; }

	// if you add anything to these enums, only add at the end!
   enum class EBank { kWFDN, kGRF1, kGRF2, kGRF3, kGRF4, kFME0, kFME1, kFME2, kFME3 };

#ifndef __CINT__
   virtual std::shared_ptr<ThreadsafeQueue<std::shared_ptr<const TFragment>>>& AddGoodOutputQueue(size_t maxSize = 50000)
	{
		std::stringstream name;
		name<<"good_frag_queue_"<<fGoodOutputQueues.size();
		fGoodOutputQueues.push_back(std::make_shared<ThreadsafeQueue<std::shared_ptr<const TFragment>>>(name.str(), maxSize));
		return fGoodOutputQueues.back();
	}

	virtual std::shared_ptr<ThreadsafeQueue<std::shared_ptr<const TBadFragment>>>& BadOutputQueue() { return fBadOutputQueue; }

	virtual std::shared_ptr<ThreadsafeQueue<std::shared_ptr<TEpicsFrag>>>& ScalerOutputQueue() { return fScalerOutputQueue; }

	virtual void SetStatusVariables(std::atomic_size_t* itemsPopped, std::atomic_long* inputSize)
	{
		fItemsPopped = itemsPopped;
		fInputSize   = inputSize;
	}

	virtual int Process(std::shared_ptr<TRawEvent>) = 0;
	void Push(ThreadsafeQueue<std::shared_ptr<const TBadFragment>>& queue, const std::shared_ptr<TBadFragment>& frag);
	void Push(std::vector<std::shared_ptr<ThreadsafeQueue<std::shared_ptr<const TFragment>>>>& queues, const std::shared_ptr<TFragment>& frag);
#endif
	virtual void        ClearQueue();
	virtual size_t      ItemsPushed()
	{
		if(fGoodOutputQueues.size() > 0) {
			return fGoodOutputQueues.back()->ItemsPushed();
		}
		return std::numeric_limits<std::size_t>::max();
	}
	virtual void        SetFinished();
	virtual std::string OutputQueueStatus();

protected:
#ifndef __CINT__
	std::vector<std::shared_ptr<ThreadsafeQueue<std::shared_ptr<const TFragment>>>> fGoodOutputQueues;
	std::shared_ptr<ThreadsafeQueue<std::shared_ptr<const TBadFragment>>>           fBadOutputQueue;
	std::shared_ptr<ThreadsafeQueue<std::shared_ptr<TEpicsFrag>>>                   fScalerOutputQueue;
#endif

	bool      fNoWaveforms; ///< The flag to turn wave_forms on or off
	bool      fRecordDiag;  ///< The flag to turn on diagnostics recording
	TChannel* fChannel;

	const unsigned long fMaxTriggerId;      ///< The last trigger ID Called
	unsigned long       fLastDaqId;       ///< The last daq ID in the raw file
	unsigned long       fLastTriggerId;     ///< The last Trigged ID in the raw File
	unsigned long       fLastNetworkPacket; ///< The last network packet recieved.

	std::map<int, int> fFragmentIdMap;
	bool fFragmentHasWaveform;

	TFragmentMap fFragmentMap;              ///< Class that holds a map of fragments per address, takes care of calculating charges for GRF4 banks

	std::map<UInt_t, Long64_t> fLastTimeStampMap;

	static TGRSIOptions* fOptions; ///< Static pointer to TGRSIOptions, gets set on the first call of GriffinDataToFragment

#ifndef __CINT__
	std::atomic_size_t* fItemsPopped;
	std::atomic_long*   fInputSize;
#endif
};
/*! @} */
#endif
