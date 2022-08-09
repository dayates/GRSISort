#include "ExampleTreeHelper.hh"

void ExampleTreeHelper::CreateHistograms(unsigned int slot) {
	// define Trees
	fTree[slot].emplace("coinc", new TTree("coinc", "coincident events"));
	fTree[slot]["timebg"] = new TTree("timebg", "time random events");

	// set branch addresses for output tree (these can be different for different trees)
	// four coincident gammas a,b,c,d will be saved as a,b,c, a,b,d, a,c,d, and b,c,d
	fTree[slot]["coinc"]->Branch("energy", &fSuppressedAddback);
	fTree[slot]["coinc"]->Branch("timing", &fBetaGammaTiming);
	fTree[slot]["coinc"]->Branch("mult", &fGriffinMultiplicity, "mult/I");

	fTree[slot]["timebg"]->Branch("energy", &fSuppressedAddback);
	fTree[slot]["timebg"]->Branch("timing", &fBetaGammaTiming);

	//We can also create histograms at the same time, maybe for some diagnostics or simple checks
	fH1[slot]["asE"]	= new TH1D("asE", "suppressed Addback Singles", 12000,0,3000);
	fH2[slot]["sceptarMult"] = new TH2D("sceptarMult", "Sceptar multiplicity vs. Griffin multiplicity", 64, 0, 64, 40, 0, 40);
	fH2[slot]["zdsMult"] = new TH2D("zdsMult", "ZeroDegree multiplicity vs. Griffin multiplicity", 64, 0, 64, 10, 0, 10);
}

bool PromptCoincidence(TGriffinHit* g, TZeroDegreeHit* z){
	//Check if hits are less then 300 ns apart.
	return std::fabs(g->GetTime() - z->GetTime()) < 300.;
}

bool PromptCoincidence(TGriffinHit* g, TSceptarHit* s){
	//Check if hits are less then 300 ns apart.
	return std::fabs(g->GetTime() - s->GetTime()) < 300.;
}

bool PromptCoincidence(TGriffinHit* g1, TGriffinHit* g2){
	//Check if hits are less then 500 ns apart.
	return std::fabs(g1->GetTime() - g2->GetTime()) < 500.;
}

void ExampleTreeHelper::Exec(unsigned int slot, TGriffin& grif, TGriffinBgo& grifBgo, TZeroDegree& zds, TSceptar& scep) {
	// we could check multiplicities here and skip events where we do not have at least
	// three suppressed addback energies and a beta, but we want to fill some general 
	// histograms without these cuts.
	
	// clear the vectors and other variables
	fSuppressedAddback.resize(3, 0.);
	fBetaGammaTiming.resize(3, -1e6);

	fH2[slot].at("sceptarMult")->Fill(grif.GetSuppressedAddbackMultiplicity(&grifBgo), scep.GetMultiplicity());
	fH2[slot].at("zdsMult")->Fill(grif.GetSuppressedAddbackMultiplicity(&grifBgo), zds.GetMultiplicity());

	//Loop over all suppressed addback Griffin Hits
	fGriffinMultiplicity = grif.GetSuppressedAddbackMultiplicity(&grifBgo);
   for(auto i = 0; i < fGriffinMultiplicity; ++i) {
		auto grif1 = grif.GetSuppressedAddbackHit(i);
      fH1[slot].at("asE")->Fill(grif1->GetEnergy());
		fSuppressedAddback[0] = grif1->GetEnergy();
		for(auto j = i+1; j < grif.GetSuppressedAddbackMultiplicity(&grifBgo); ++j) {
			auto grif2 = grif.GetSuppressedAddbackHit(j);
			fSuppressedAddback[1] = grif2->GetEnergy();
			for(auto k = j+1; k < grif.GetSuppressedAddbackMultiplicity(&grifBgo); ++k) {
				auto grif3 = grif.GetSuppressedAddbackHit(k);
				fSuppressedAddback[2] = grif3->GetEnergy();
				// we now have three suppressed addback hits i, j, and k so now we need a coincident beta-tag
				bool foundBeta = false;
				for(auto b = 0; b < zds.GetMultiplicity(); ++b) {
					auto beta = zds.GetZeroDegreeHit(b);
					if(b == 0) {
						// use the time of the first beta as reference in case we don't find a coincident beta
						fBetaGammaTiming[0] = grif1->GetTime() - beta->GetTime();
						fBetaGammaTiming[1] = grif2->GetTime() - beta->GetTime();
						fBetaGammaTiming[2] = grif3->GetTime() - beta->GetTime();
					}
					if(PromptCoincidence(grif1, beta) && PromptCoincidence(grif2, beta) && PromptCoincidence(grif3, beta)) {
						foundBeta = true;
						fBetaGammaTiming[0] = grif1->GetTime() - beta->GetTime();
						fBetaGammaTiming[1] = grif2->GetTime() - beta->GetTime();
						fBetaGammaTiming[2] = grif3->GetTime() - beta->GetTime();
						break;
					}
				}
				// only check sceptar if we haven't found a zds yet
				for(auto b = 0; !foundBeta && b < scep.GetMultiplicity(); ++b) {
					auto beta = scep.GetSceptarHit(b);
					if(b == 0) {
						// use the time of the first beta as reference in case we don't find a coincident beta
						fBetaGammaTiming[0] = grif1->GetTime() - beta->GetTime();
						fBetaGammaTiming[1] = grif2->GetTime() - beta->GetTime();
						fBetaGammaTiming[2] = grif3->GetTime() - beta->GetTime();
					}
					if(PromptCoincidence(grif1, beta) && PromptCoincidence(grif2, beta) && PromptCoincidence(grif3, beta)) {
						foundBeta = true;
						fBetaGammaTiming[0] = grif1->GetTime() - beta->GetTime();
						fBetaGammaTiming[1] = grif2->GetTime() - beta->GetTime();
						fBetaGammaTiming[2] = grif3->GetTime() - beta->GetTime();
						break;
					}
				}

				if(foundBeta) {
					auto entries = fTree[slot].at("coinc")->GetEntries();
					if(entries < 10) {
						std::cout<<slot<<" "<<fGriffinMultiplicity<<" "<<fSuppressedAddback.size();
						for(auto e : fSuppressedAddback) {
							std::cout<<" "<<e;
						}
						std::cout<<std::endl;
					}
					fTree[slot].at("coinc")->Fill();
					if(entries < 10) {
						fTree[slot].at("coinc")->Scan("*");
					}
				} else {
					fTree[slot].at("timebg")->Fill();
				}
			}
		}
	}
}
