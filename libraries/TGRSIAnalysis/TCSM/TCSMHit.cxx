
#include "TCSMHit.h"

ClassImp(TCSMHit)

TCSMHit::TCSMHit()	{	
   Class()->IgnoreTObjectStreamer(true);
   Clear();
}

TCSMHit::~TCSMHit()	{	}


void TCSMHit::Clear(Option_t *options)	{

   hor_d_strip 	= -1;
   hor_d_charge = 0;
   hor_d_cfd    = 0.0;
	
   ver_d_strip  = -1;
   ver_d_charge	= 0;
   ver_d_cfd	= 0.0;      
	
   hor_e_strip  = -1;
   hor_e_charge = 0;
   hor_e_cfd    = 0.0;
	
   ver_e_strip  = -1;
   ver_e_charge = 0;
   ver_e_cfd    = 0.0;      

   hor_d_energy = 0.0;   
   ver_d_energy = 0.0;   
   hor_d_time   = 0.0;   
   ver_d_time   = 0.0;   
   d_position.SetXYZ(0,0,1);
	
   hor_e_energy = 0.0;
   ver_e_energy = 0.0;
   hor_e_time   = 0.0;
   ver_e_time   = 0.0;
   e_position.SetXYZ(0,0,1);

   detectornumber = 0;	//
}

void TCSMHit::Print(Option_t *options)	{
  /*
  printf(DGREEN "D: [D/F/B] = %02i\t/%02i\t/%02i " RESET_COLOR "\n",detectornumber,hor_d_strip,ver_d_strip);
  printf(DGREEN "E: [D/F/B] = %02i\t/%02i\t/%02i " RESET_COLOR "\n",detectornumber,hor_e_strip,ver_e_strip);
  //GetDetectorNumber(),GetHorizontalStrip(),GetVerticalStrip());
  printf("D: CSM hit charge: %f\t0x%08x\n",(double)hor_d_charge/125.0,hor_d_charge);
  printf("E: CSM hit charge: %f\t0x%08x\n",(double)hor_e_charge/125.0,hor_e_charge);
  //printf("CSM hit energy: %f\n",d_energy);
  //printf("CSM hit time:   %f\n",d_time);
  printf( DGREEN "=	=	=	=	=	=	=	" RESET_COLOR "\n");
  */
  printf(DGREEN "\n\t\t****Printing TCSMHit****" RESET_COLOR "\n");
  printf(DGREEN "\t****Delta Info****" RESET_COLOR "\n");
  printf(DGREEN "\t\t Vert    Horiz" RESET_COLOR "\n");
  printf(DBLUE "\tStrip:\t" RESET_COLOR "   %i       %i" RESET_COLOR "\n", ver_d_strip,hor_d_strip);
  printf(DBLUE "\tCharge:\t" RESET_COLOR "   %d       %d" RESET_COLOR "\n", ver_d_charge,hor_d_charge);
  printf(DBLUE "\tTime:\t" RESET_COLOR "   %d       %lf" RESET_COLOR "\n", ver_d_time,hor_d_time);
  printf(DBLUE "\tCFD:\t" RESET_COLOR "   %d       %lf" RESET_COLOR "\n", ver_d_cfd,hor_d_cfd);
}

