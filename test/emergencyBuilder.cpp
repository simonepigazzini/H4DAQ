#include "interface/StandardIncludes.hpp"
#include "interface/Utility.hpp"
#include "interface/EventBuilder.hpp"
#include "interface/FindMatch.hpp"
#include "interface/DataType.hpp"
#include "interface/Board.hpp"

#ifndef NO_ROOT
	#include "TPad.h"
	#include "TCanvas.h"
	#include "TH1D.h"
	#include "TROOT.h"
	#include "TStyle.h"
	#include "TLegend.h"
#endif

int main(int argc, char**argv) 
{
 // This is an emerency program for merging two spills 
//
printf("Usage ./bin/... file1 file2 outfile. Please make me configurable.\n");
if (argc <3 ) return 0;
 
FindMatch A;
A.SetMaxWindow(1); 

vector<uint64_t> time1,time2,time3;

string fileName1=argv[1];
string fileName2=argv[2];
string outFileName=argv[3];

dataType file1;
dataType file2;

dataType mySpill;

// Read Files and move them into datatype
//h
FILE *fr1= fopen(fileName1.c_str(),"rb");
FILE *fr2= fopen(fileName2.c_str(),"rb");

char c;
while ( fscanf(fr1,"%c",&c) != EOF ) file1.append((void*)&c,1);
while ( fscanf(fr2,"%c",&c) != EOF ) file2.append((void*)&c,1);

fclose(fr1);
fclose(fr2);

// get list of times
// go trough
	WORD runNum1 = EventBuilder::ReadRunNumFromSpill(file1);
	WORD runNum2 = EventBuilder::ReadRunNumFromSpill(file2);

	WORD spillNum1 = EventBuilder::ReadSpillNum(file1);
	WORD spillNum2 = EventBuilder::ReadSpillNum(file2);

	WORD spillNevents1 = EventBuilder::ReadSpillNevents(file1);
	WORD spillNevents2 = EventBuilder::ReadSpillNevents(file2);

	WORD zero=0; //spillSize

	if (runNum1 != runNum2) { printf("Different Run Number: I will proceed using the first.\n"); } // 
	if (spillNum1 != spillNum2){ printf("Different SpillN: I will proceed using the first.\n"); } 
	if (spillNevents1 != spillNevents2){printf("Different n. event in spill. I will try to match.\n");  }


	// navigate through to find the numbers and times
	char *ptr1 = (char*)file1.data();
	char *ptr2 = (char*)file2.data();
	ptr1 += WORDSIZE*EventBuilder::SpillHeaderWords();
	ptr2 += WORDSIZE*EventBuilder::SpillHeaderWords();
	long long left1 = file1.size() - WORDSIZE*EventBuilder::SpillHeaderWords();
	long long left2 = file2.size() - WORDSIZE*EventBuilder::SpillHeaderWords();

	// save here the positions of the events
	vector<char*> myEvents1;
	vector<long>  myEventSize1;

	vector<char*> myEvents2;
	vector<long>  myEventSize2;

	for(unsigned long long iEvent=0;iEvent< spillNevents1 ;iEvent++)
	{
		printf("event1, check Event # %llu\n",iEvent);
		long eventSize1= EventBuilder::IsEventOk(ptr1,left1);
		if (eventSize1 == 0 ) {
				printf("event %llu has size 0\n",iEvent);
				printf("event %c%c%c%c\n\n",ptr1[0],ptr1[1],ptr1[2],ptr1[3]);
				return 2;
				}
		WORD tB=(WORD)_TIME_;
		WORD bId1 = * (WORD*)(ptr1+ (EventBuilder::EventTimePos()-2)*WORDSIZE );
		if ( (bId1 &EventBuilder::GetBoardTypeIdBitMask () ) >> EventBuilder::GetBoardTypeShift() != tB )  return 3;
		uint64_t mytime1 = *( (uint64_t*) (ptr1 + EventBuilder::EventTimePos()*WORDSIZE)   );  // carefull to parenthesis
		time1.push_back(mytime1);
		myEvents1.push_back(ptr1);
		myEventSize1.push_back(eventSize1);
		ptr1+= eventSize1;
		left1-=eventSize1;
	}
	for(unsigned long long iEvent=0;iEvent< spillNevents2 ;iEvent++)
	{
		printf("event2, check Event # %llu\n",iEvent);
		long eventSize2= EventBuilder::IsEventOk(ptr2,left2);
		if (eventSize2 == 0 ) {
			printf("event %llu has size 0\n",iEvent);
			return 4;
			}
		WORD tB=(WORD)_TIME_;
		WORD bId2 = * (WORD*)(ptr2+ (EventBuilder::EventTimePos()-2)*WORDSIZE );
		if ( (bId2 &EventBuilder::GetBoardTypeIdBitMask () ) >> EventBuilder::GetBoardTypeShift() != tB )  return 5;
		uint64_t mytime2 = *( (uint64_t*) (ptr2 + EventBuilder::EventTimePos()*WORDSIZE)   );  // carefull to parenthesis
		time2.push_back(mytime2);
		myEvents2.push_back(ptr2);
		myEventSize2.push_back(eventSize2);
		ptr2+= eventSize2;
		left2-=eventSize2;
	}

	// Run Matching Algo
	//
	printf("Going to Run Matching %u - %u\n",time1.size(),time2.size());
	A.SetTimes(time1,time2);
	timeval tv_start; 
	gettimeofday(&tv_start,NULL);
	//int status=A.Run();
	int status=A.Iterative();
	if (status >0 ) return status+100;
	timeval tv_stop;
	gettimeofday(&tv_stop,NULL);

	printf("     Converged=%d\n", int(A.Converged()) ) ;
	if( !A.Converged() ) {printf("Not Converged\n");return 200;}
	printf("USER TIME: %ld usec\n", Utility::timevaldiff( &tv_start, &tv_stop) );
	printf("     status=%d == 0\n", status ) ;
	printf("     alpha=%lf\n", A.GetAlpha() ) ;
	printf("     Window (If SLOW REDUCE)=%d\n", A.GetMaxWindow() ) ;
	printf("     d=%lf\n", A.GetDistance() ) ;
	printf("     d2=%lf\n", A.GetDistance2() ) ;
	printf("     maxChi2=%lf\n", A.GetMaxChi2() ) ;

	vector<pair<uint_t,uint_t> > matched=A.GetMatch(); // positions in time1/time2
	printf("     Matched size=%u\n", matched.size() ) ;

	

	dataType H;EventBuilder::SpillHeader(H);
	dataType T;EventBuilder::SpillTrailer(T);

	mySpill.append(H);
	mySpill.append((void*)&runNum1,WORDSIZE); // 
	mySpill.append((void*)&spillNum1,WORDSIZE); // 
	mySpill.append( (void*)&zero, WORDSIZE); // spillSize
	mySpill.append((void*)&spillNevents1,WORDSIZE); // this must be updated
	for(unsigned int iEvent=0; iEvent< matched.size() ;++iEvent)
	{
		//printf("merging event %u with event %u\n",matched[iEvent].first,matched[iEvent].second);
		dataType event1(myEventSize1[matched[iEvent].first],myEvents1[matched[iEvent].first]);
		dataType event2(myEventSize2[matched[iEvent].second],myEvents2[matched[iEvent].second]);
		dataType myEvent;
		EventBuilder::MergeEventStream(myEvent,event1,event2);
		event1.release();
		event2.release();
		mySpill.append(myEvent);
	}
	// trailer
	mySpill.append(T);
	/// update size
	WORD *spillSizePtr= ((WORD*) mySpill.data() )+ EventBuilder::SpillSizePos();
	(*spillSizePtr)=(WORD)mySpill.size();
	WORD *spillNevtPtr= ((WORD*) mySpill.data() )+ EventBuilder::SpillNeventPos();
	(*spillNevtPtr)=(WORD) matched.size();
	printf("MATCHED %u EVENTS\n",matched.size());
	// write on disk
	FILE *fw=fopen(outFileName.c_str(),"wb");
	fwrite(mySpill.data(),1,mySpill.size(),fw);
	fflush(fw);
	fclose(fw);
#ifndef NO_ROOT
	{
		gStyle->SetOptStat(0);
		TCanvas *c=new TCanvas("c","c",800,800);
		int nBins=100000;
		double maxX=1000000;
		TH1D *h1 =new TH1D("time1","time1;time;time1",nBins,0,maxX);
		TH1D *h2 =new TH1D("time2","time2;time;time2",nBins,0,maxX);
		//TH1D *m1 =new TH1D("matched1","matched1;time;time1",nBins,0,maxX);
		//TH1D *m2 =new TH1D("matched2","matched2;time;time2",nBins,0,maxX);
		TH1D *h0 =new TH1D("matched2_1" ,"shifted;time;#Delta time" ,nBins,0,maxX);
		// plot wrt a reference
		uint64_t ref=time1[0];
		if (time2[0]< ref) ref=time2[0];
		ref += 2;
		//TPad *p0= new TPad("p0","p0",0,0,1,0.5); p0->SetTopMargin(0);       p0->SetBottomMargin(.05/.50);
		//TPad *p1= new TPad("p1","p1",0,0.5,1,1); p1->SetTopMargin(.05/.50); p1->SetBottomMargin(0);
		TPad *p0= new TPad("p0","p0",0,0.0,1,.2); p0->SetTopMargin(0); p0->SetBottomMargin(.5);
		TPad *p1= new TPad("p1","p1",0,0.2,1,.3); p1->SetTopMargin(0); p1->SetBottomMargin(0);
		TPad *p2= new TPad("p2","p2",0,0.3,1,.4); p2->SetTopMargin(0); p2->SetBottomMargin(0);
		TPad *p3= new TPad("p3","p3",0,0.4,1,.5); p3->SetTopMargin(0); p3->SetBottomMargin(0);
		TPad *p4= new TPad("p4","p4",0,0.5,1,.6); p4->SetTopMargin(0); p4->SetBottomMargin(0);
		TPad *p5= new TPad("p5","p5",0,0.6,1,.7); p5->SetTopMargin(0); p5->SetBottomMargin(0);
		TPad *p6= new TPad("p6","p6",0,0.7,1,.8); p6->SetTopMargin(0); p6->SetBottomMargin(0);
		TPad *p7= new TPad("p7","p7",0,0.8,1,1.); p7->SetTopMargin(.5); p7->SetBottomMargin(0);
		p0->Draw();
		p1->Draw();
		p2->Draw();
		p3->Draw();
		p4->Draw();
		p5->Draw();
		p6->Draw();
		p7->Draw();
		for(unsigned int i=0;i<time1.size();++i) h1->Fill( time1[i] - time1[0] , 0.6);
		for(unsigned int i=0;i<time2.size();++i) h2->Fill( time2[i] - time2[0] , 0.3);
		double delta=0;
		for(unsigned int i=0;i<matched.size();++i) delta += int64_t(time1[matched[i].first]) - int64_t(time2[matched[i].second]);
		delta /= matched.size();
		for(unsigned int i=0;i<matched.size();++i) h0->Fill( int64_t(time2[ matched[i].second]) +delta -time1[0], 1.0);
		h0->SetLineColor(kGreen);
		h2->SetLineColor(kRed);

		//p0->cd();
		//h1->Draw("HIST");

		p7->cd();
		h0->GetXaxis()->SetRangeUser(     0,62500-1);
		h0->DrawCopy("HIST");
		h1->Draw("HIST SAME");
		h2->Draw("HIST SAME");
		TLegend *l=new TLegend(0.35,.5,.65,.98);
			l->SetFillColor(0);
			l->SetFillStyle(0);
			l->SetBorderSize(0);
			l->AddEntry(h0,"Merged","F");
			l->AddEntry(h1,"Time1","F");
			l->AddEntry(h2,"Time2","F");
			l->Draw();
		p6->cd();
		h0->GetXaxis()->SetRangeUser(62500,125000-1);
		h0->DrawCopy("HIST");
		h1->Draw("HIST SAME");
		h2->Draw("HIST SAME");
		p5->cd();
		h0->GetXaxis()->SetRangeUser(125000,187500-1);
		h0->DrawCopy("HIST");
		h1->Draw("HIST SAME");
		h2->Draw("HIST SAME");
		p4->cd();
		h0->GetXaxis()->SetRangeUser(187500,250000-1);
		h0->DrawCopy("HIST");
		h1->Draw("HIST SAME");
		h2->Draw("HIST SAME");
		p3->cd();
		h0->GetXaxis()->SetRangeUser(250000,312500-1);
		h0->DrawCopy("HIST");
		h1->Draw("HIST SAME");
		h2->Draw("HIST SAME");
		p2->cd();
		h0->GetXaxis()->SetRangeUser(312500,375000-1);
		h0->DrawCopy("HIST");
		h1->Draw("HIST SAME");
		h2->Draw("HIST SAME");
		p1->cd();
		h0->GetXaxis()->SetRangeUser(375000,437500-1);
		h0->DrawCopy("HIST");
		h1->Draw("HIST SAME");
		h2->Draw("HIST SAME");
		p0->cd();
		h0->GetXaxis()->SetRangeUser(437500,500000-1);
		h0->DrawCopy("HIST");
		h1->Draw("HIST SAME");
		h2->Draw("HIST SAME");
		c->SaveAs( (outFileName + ".pdf").c_str() ) ;

	}
#endif
	return 0;
}
