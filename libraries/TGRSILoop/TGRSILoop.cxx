
#include <stdint.h>

#include "TGRSILoop.h"
#include "TGRSIOptions.h"
#include "TDataParser.h"

#include "TFragmentQueue.h"
#include "TGRSIRootIO.h"
#include "TGRSIStats.h"

ClassImp(TGRSILoop)

TGRSILoop *TGRSILoop::fTGRSILoop = 0;

bool TGRSILoop::suppress_error = false;

TGRSILoop *TGRSILoop::Get() {
   if(!fTGRSILoop)
      fTGRSILoop = new TGRSILoop();
   return fTGRSILoop;
}

TGRSILoop::TGRSILoop()   { 

   //intiallize flags
   fOffline    = true;
   fTestMode   = false;

   fMidasThreadRunning    = false;
   fFillTreeThreadRunning = false;

   fFragsSentToTree = 0;

   fMidasThread = 0;
   fFillTreeThread = 0;
   fOdb = 0;
}

TGRSILoop::~TGRSILoop()  {  }

void TGRSILoop::BeginRun(int transition,int runnumber,int time)   { 
  if(fOffline)    {
  }
  //fMidasThreadRunning = true;
}

void TGRSILoop::EndRun(int transition,int runnumber,int time)     { 


   fMidasThreadRunning = false;

   if(fFillTreeThread) {

      //printf("\n\nJoining Fill Tree Thread.\n\n");
      fFillTreeThread->join();
      fFillTreeThreadRunning = false;
      //printf("\n\nFinished Fill Tree Thread.\n\n");
      delete fFillTreeThread; fFillTreeThread = 0;
   }

   //printf("\n\nFragments in que = %i \n\n",TFragmentQueue::GetQueue()->FragsInQueue());

   TGRSIRootIO::Get()->CloseRootOutFile();  

}


bool TGRSILoop::SortMidas() {
   if(fMidasThread) //already sorting.
      return true;
    
    if(TGRSIOptions::Get()->GetInputMidas().size()>0)  { //we have offline midas files to sort.
      TMidasFile *mfile = new TMidasFile;
      for(int x=0;x<TGRSIOptions::Get()->GetInputMidas().size();x++) {
         if(mfile->Open(TGRSIOptions::Get()->GetInputMidas().at(x).c_str()))  {
            //std::sting filename = mfile->GetName();
            fMidasThread = new std::thread(&TGRSILoop::ProcessMidasFile,this,mfile);
            fMidasThreadRunning = true;
            fFillTreeThread = new std::thread(&TGRSILoop::FillFragmentTree,this,mfile);
            fFillTreeThreadRunning = true;
            //printf("\n\nJoining Midas Thread.\n\n");
            fMidasThread->join();
            //printf("\n\nFinished Midas Thread.\n\n");
  //          printf("\n");
            delete fMidasThread; fMidasThread = 0;
   
          }
      }
      mfile->Close();
      delete mfile;
   }
   TGRSIOptions::Get()->GetInputMidas().clear();
   return true;
}


void TGRSILoop::FillFragmentTree(TMidasFile *midasfile) {

   if(!TGRSIRootIO::Get()->GetRootOutFile())
      TGRSIRootIO::Get()->SetUpRootOutFile(midasfile->GetRunNumber(),midasfile->GetSubRunNumber());

   
   fFragsSentToTree = 0;
   TFragment *frag = 0;
   while(TFragmentQueue::GetQueue()->FragsInQueue() !=0 || fMidasThreadRunning)
   {
      frag = TFragmentQueue::GetQueue()->PopFragment();
      if(frag) {
         TGRSIRootIO::Get()->FillFragmentTree(frag);
 	    delete frag;
         fFragsSentToTree++;
      }
      if(!fMidasThreadRunning && TFragmentQueue::GetQueue()->FragsInQueue()%5000==0) {
         printf(DYELLOW "\t%i" RESET_COLOR "/"
                DBLUE   "%i"   RESET_COLOR
                "     frags left to write to tree/frags written to tree.         \r",
                TFragmentQueue::GetQueue()->FragsInQueue(),fFragsSentToTree);
      }
   }




   printf("\n");
   //printf(" \n\n quiting fill tree thread \n\n");
   return;
}

void TGRSILoop::ProcessMidasFile(TMidasFile *midasfile) {
   if(!midasfile)
      return;

   fOffline = true;

   std::ifstream in(midasfile->GetFilename(), std::ifstream::in | std::ifstream::binary);
   in.seekg(0, std::ifstream::end);
   long long filesize = in.tellg();
   in.close();

   TMidasEvent fMidasEvent;

   Initialize();
   fMidasEvent.Clear();

   int bytes = 0;
   long long bytesread = 0;
   int currenteventnumber = 0;

   while(true) {
      bytes = midasfile->Read(&fMidasEvent);
      currenteventnumber++;
      if(bytes == 0){
         printf(DMAGENTA "\tfile: %s ended on %s" RESET_COLOR "\n",midasfile->GetFilename(),midasfile->GetLastError());
         break;
      }
      bytesread += bytes;
      int eventId = fMidasEvent.GetEventId();
      switch(eventId)   {
         case 0x8000:
            printf( DGREEN );
            fMidasEvent.Print();
            printf( RESET_COLOR );
            BeginRun(0,0,0);
            SetFileOdb(fMidasEvent.GetData(),fMidasEvent.GetDataSize());
            break;
         case 0x8001:
            printf("Processing event %i have processed %.2fMB/%.2fMB\n",currenteventnumber,(bytesread/1000000.0),(filesize/1000000.0));
            printf( DRED );
            fMidasEvent.Print();
            printf( RESET_COLOR );
            EndRun(0,0,0);
            break;
         default:
            fMidasEvent.SetBankList();
            ProcessMidasEvent(&fMidasEvent);
            break;
      };
      if((currenteventnumber%5000)== 0) {
         printf("Processing event %i have processed %.2fMB/%.2fMB                \r",currenteventnumber,(bytesread/1000000.0),(filesize/1000000.0));
      }
   }
   if(TGRSIStats::GetSize() > 0) {
	CreateStatsLog(midasfile->GetRunNumber(),midasfile->GetSubRunNumber());
   }

   Finalize();
   return;
}


void TGRSILoop::SetFileOdb(char *data, int size) {
   //chaeck if we have already set the tchannels....
   //
   if(!fOdb) 
      fOdb = new TXMLOdb(data,size);

   TXMLNode *node = fOdb->FindPath("/Experiment");
   if(!node->HasChildren())
      return;
   node = node->GetChildren();
   std::string expt;
   while(1) {
      std::string key = fOdb->GetNodeName(node);
      if(key.compare("Name")==0) {
         expt = node->GetText();
         break;
      }
      if(!node->HasNextNode())
         break;
      node = node->GetNextNode();
   }
   if(expt.compare("tigress")==0)
      SetTIGOdb();
   else if(expt.compare("griffin")==0)
      SetGRIFFOdb();
}

void TGRSILoop::SetGRIFFOdb() {
   std::string path = "/DAQ/MSC"; 
   printf("Using GRIFFIN path to analyzer info: %s...\n",path.c_str());
   
   std::string temp = path; temp.append("/MSC");
   TXMLNode *node = fOdb->FindPath(temp.c_str());
   std::vector<int> address = fOdb->ReadIntArray(node);

   temp = path; temp.append("/chan");
   node = fOdb->FindPath(temp.c_str());
   std::vector<std::string> names = fOdb->ReadStringArray(node);

   temp = path; temp.append("/datatype");
   node = fOdb->FindPath(temp.c_str());
   std::vector<int> type = fOdb->ReadIntArray(node);

   temp = path; temp.append("/gain");
   node = fOdb->FindPath(temp.c_str());
   std::vector<double> gains = fOdb->ReadDoubleArray(node);
   
   temp = path; temp.append("/offset");
   node = fOdb->FindPath(temp.c_str());
   std::vector<double> offsets = fOdb->ReadDoubleArray(node);


   if( (address.size() == names.size()) && (names.size() == gains.size()) && (gains.size() == offsets.size()) && offsets.size() == type.size() ) {
      //all good.
   }  else {
      printf(BG_WHITE DRED "problem parsing odb data, arrays are different sizes, channels not set." RESET_COLOR "\n");
      return;
   }

   for(int x=0;x<address.size();x++) {
      TChannel *tempchan = TChannel::GetChannel(address.at(x));   //names.at(x).c_str());
      tempchan->SetChannelName(names.at(x).c_str());
      tempchan->SetAddress(address.at(x));
      tempchan->SetNumber(x);
      //printf("temp chan(%s) number set to: %i\n",tempchan->GetChannelName(),tempchan->GetNumber());
      
      tempchan->SetUserInfoNumber(x);
      tempchan->AddENGCoefficient(offsets.at(x));
      tempchan->AddENGCoefficient(gains.at(x));
   } 
   printf("\t%i TChannels created.\n",TChannel::GetNumberOfChannels());

   return;
}

void TGRSILoop::SetTIGOdb()  {
  
   std::string typepath = "/Equipment/Trigger/settings/Detector Settings";
   std::map<int,std::pair<std::string,std::string> >typemap;
   TXMLNode *typenode = fOdb->FindPath(typepath.c_str());
   int typecounter = 0;
   if(typenode->HasChildren()) {
      TXMLNode *typechild = typenode->GetChildren();
      while(1) {
         std::string tname = fOdb->GetNodeName(typechild);
         if(tname.length()>0 && typechild->HasChildren()) {
            typecounter++;
            TXMLNode *grandchild = typechild->GetChildren();
            while(1) {
               std::string grandchildname = fOdb->GetNodeName(grandchild);
               if(grandchildname.compare(0,7,"Digitis")==0) {
                  std::string dname = grandchild->GetText();
                  typemap[typecounter] = std::make_pair(tname,dname);
                  break;
               }
               if(!grandchild->HasNextNode())
                  break;
               grandchild = grandchild->GetNextNode();
            }
         }
         if(!typechild->HasNextNode())
            break;
         typechild = typechild->GetNextNode();
      }
 
   }
   
   std::string path = "/Analyzer/Shared Parameters/Config";
   printf("Using TIGRESS path to analyzer info: %s...\n",path.c_str());

   std::string temp = path; temp.append("/FSCP");
   TXMLNode *node = fOdb->FindPath(temp.c_str());
   std::vector<int> address = fOdb->ReadIntArray(node);

   temp = path; temp.append("/Name");
   node = fOdb->FindPath(temp.c_str());
   std::vector<std::string> names = fOdb->ReadStringArray(node);

   temp = path; temp.append("/Type");
   node = fOdb->FindPath(temp.c_str());
   std::vector<int> type = fOdb->ReadIntArray(node);

   temp = path; temp.append("/g");
   node = fOdb->FindPath(temp.c_str());
   std::vector<double> gains = fOdb->ReadDoubleArray(node);
   
   temp = path; temp.append("/o");
   node = fOdb->FindPath(temp.c_str());
   std::vector<double> offsets = fOdb->ReadDoubleArray(node);


   if( (address.size() == names.size()) && (names.size() == gains.size()) && (gains.size() == offsets.size()) && offsets.size() == type.size() ) {
      //all good.
   }  else {
      printf(BG_WHITE DRED "problem parsing odb data, arrays are different sizes, channels not set." RESET_COLOR "\n");
      return;
   }

   for(int x=0;x<address.size();x++) {
      TChannel *tempchan = TChannel::GetChannel(address.at(x));   //names.at(x).c_str());
      tempchan->SetChannelName(names.at(x).c_str());
      tempchan->SetAddress(address.at(x));
      tempchan->SetNumber(x);
      //printf("temp chan(%s) number set to: %i\n",tempchan->GetChannelName(),tempchan->GetNumber());
      if(type.at(x) != 0) {
         tempchan->SetTypeName(typemap[type.at(x)].first);
         tempchan->SetDigitizerType(typemap[type.at(x)].second);
      }
      
      tempchan->SetUserInfoNumber(x);
      tempchan->AddENGCoefficient(offsets.at(x));
      tempchan->AddENGCoefficient(gains.at(x));
   } 
   printf("\t%i TChannels created.\n",TChannel::GetNumberOfChannels());
   return;
}

bool TGRSILoop::ProcessMidasEvent(TMidasEvent *mevent)   {
   if(!mevent)
      return false;
   int banksize;
   void *ptr;
   try {
      switch(mevent->GetEventId())  {
         case 1:
            if((banksize = mevent->LocateBank(NULL,"WFDN",&ptr))>0) {
	       if(!ProcessTIGRESS((uint32_t*)ptr, banksize, mevent)) { }
                              //(unsigned int)(mevent->GetSerialNumber()),
                              //(unsigned int)(mevent->GetTimeStamp()))) { }
            }
            else if((banksize = mevent->LocateBank(NULL,"GRF1",&ptr))>0) {
               if(!ProcessGRIFFIN((uint32_t*)ptr,banksize, mevent)) { }
			      //(unsigned int)(mevent->GetSerialNumber()),
			      //(unsigned int)(mevent->GetTimeStamp()))) { }
            }
      };
   }
   catch(const std::bad_alloc&) {   }
   return true;

}

void TGRSILoop::Initialize() {   }

void TGRSILoop::Finalize() { 
   printf("in finalization phase.\n");   
//   TIter *iter = TChannel::GetChannelIter();   
//   while(TChannel *chan = (TChannel*)iter->Next()) {
//      TGRSIRootIO::Get()->FillChannelTree(chan);
//      TGRSIRootIO::Get()->GetChannelTree()->GetUserInfo()->Add(chan);
//   }
//   TGRSIRootIO::Get()->CloseRootOutFile();
}


bool TGRSILoop::ProcessEPICS() { //TMidasEvent *mevent)   {
   return true;
}


bool TGRSILoop::ProcessTIGRESS(uint32_t *ptr, int &dsize, TMidasEvent *mevent)   {
	unsigned int mserial=0; if(mevent) mserial = (unsigned int)(mevent->GetSerialNumber());
	unsigned int mtime=0;   if(mevent) mtime   = (unsigned int)(mevent->GetTimeStamp());
	int frags = TDataParser::TigressDataToFragment(ptr,dsize,mserial,mtime);
	if(frags>-1)
	   return true;
	else	{
	   if(!suppress_error && mevent)  mevent->Print(Form("a%i",(-1*frags)-1));
	   return false;
	}
}

bool TGRSILoop::ProcessGRIFFIN(uint32_t *ptr, int &dsize, TMidasEvent *mevent)   {
	unsigned int mserial=0; if(mevent) mserial = (unsigned int)(mevent->GetSerialNumber());
	unsigned int mtime=0;   if(mevent) mtime   = (unsigned int)(mevent->GetTimeStamp());
	int frags = TDataParser::GriffinDataToFragment(ptr,dsize,mserial,mtime);
	if(frags>-1)	{
	   return true;
	} else {	       
	   if(!suppress_error) {
		   printf(DRED "\n//**********************************************//" RESET_COLOR "\n");
		   printf(DRED "\nBad things are happening. Failed on datum %i" RESET_COLOR "\n", (-1*frags)-1);
	    	   if(mevent)  mevent->Print(Form("a%i",(-1*frags)-1));
		   printf(DRED "\n//**********************************************//" RESET_COLOR "\n");
	   }
	   return false;
	}
}



void TGRSILoop::CreateStatsLog(int runnumber, int subrunnumber) {
   std::map<int,TGRSIStats*>::iterator iter;
   ofstream statsout;
   statsout.open(Form("stats%05i_%03i.log",runnumber,subrunnumber));
   statsout << "\nRun time to the nearest second = " << TGRSIStats::GetRunTime()  << std::endl << std::endl;
   for(iter = TGRSIStats::GetMap()->begin();iter!=TGRSIStats::GetMap()->end();iter++) {
	TChannel *chan = TChannel::GetChannel(iter->second->GetAddress());
	TGRSIStats *stat = iter->second;
	statsout << "0x"<< std::hex <<  stat->GetAddress() << std::dec  << "\t" <<  chan->GetChannelName() << "\tDeadtime: " << ((float)(stat->GetDeadTime()))*(1E-9) << " seconds." << std::endl;
   }
  TGRSIStats::GetMap()->clear();
  statsout <<  std::endl;
  statsout.close();
  return;
}


void TGRSILoop::Print(Option_t *opt) {   }


void TGRSILoop::Clear(Option_t *opt) {   }








