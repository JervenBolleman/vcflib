#include "Variant.h"
#include "split.h"
#include "cdflib.hpp"
#include "pdflib.hpp"

#include <string>
#include <iostream>
#include <math.h>  
#include <cmath>
#include <stdlib.h>
#include <time.h>
#include <stdio.h>
#include <getopt.h>

using namespace std;
using namespace vcf;
struct pop{
  string   seqid;
  long int pos;

  double nalt ;
  double nref ;
  double af   ; 
  double nhomr;
  double nhoma;
  double nhet ;
  double ngeno;
  double fis  ;

  vector<int>    geno_index ;
  vector<string> genotypes  ;
  vector< vector< double > > unphred_p;  
  
};

double unphred(string phred){  
  double unphred = atof(phred.c_str());
  unphred = unphred / -10;
  return unphred;
}

void initPop(pop & population){
  population.nalt  = 0;
  population.nref  = 0;
  population.af    = 0;
  population.nhomr = 0;
  population.nhoma = 0;
  population.nhet  = 0;
  population.ngeno = 0;
  population.fis   = 0;
  
}

void loadPop( vector< map< string, vector<string> > >& group, pop & population, string seqid, long int pos, int phased ){

  vector< map< string, vector<string> > >::iterator targ_it = group.begin();
  
  population.seqid = seqid;
  population.pos   = pos  ;
    
  for(; targ_it != group.end(); targ_it++){
    
    string genotype = (*targ_it)["GT"].front();
    
    vector<double> phreds;
    
    double sum  = 0;
 
    if(phased == 0){
      if(genotype != "./."){
	
	double pa  = exp( unphred((*targ_it)["PL"][0])) ; 
	double pab = exp( unphred((*targ_it)["PL"][1])) ; 
	double pbb = exp( unphred((*targ_it)["PL"][2])) ; 
	
	double norm = pa + pab + pbb  ;
	
	sum += (pa  / norm);
	phreds.push_back(sum);
	sum += (pab / norm);
	phreds.push_back(sum);
	sum += (pbb / norm);
	phreds.push_back(sum);
      }
      else{
	phreds.push_back(1/3);
	phreds.push_back(2/3);
	phreds.push_back(1);
      }
    }

    population.unphred_p.push_back(phreds);
 
    population.genotypes.push_back(genotype);

    while(1){
      if(genotype == "./."){
      	population.geno_index.push_back(-1);
	break;
      }
      if(genotype == "0/0"){
	population.ngeno += 1;
	population.nhomr += 1;
	population.nref  += 2;
	population.geno_index.push_back(0);
	break;
      }
      if(genotype == "0/1"){
	population.ngeno += 1;
	population.nhet  += 1;
	population.nref  += 1;
	population.nalt  += 1;
	population.geno_index.push_back(1);
	break;
      }
      if(genotype == "1/1"){
	population.ngeno += 1;
	population.nhoma += 1;
	population.nalt  += 2;
	population.geno_index.push_back(2);
	break;
      }
      if(genotype == "0|0"){
	population.ngeno += 1;
	population.nhomr += 1;
	population.nref  += 2;
	population.geno_index.push_back(0);
	break;
      }
      if(genotype == "0|1"){
	population.ngeno += 1;
	population.nhet  += 1;
	population.nref  += 1;
	population.nalt  += 1;
	population.geno_index.push_back(1);
	break;
      }
      if(genotype == "1|0"){                                                                                          
        population.ngeno += 1;                                                                                        
        population.nhet  += 1;                                                                                        
        population.nref  += 1;                                                                                        
        population.nalt  += 1;                                                                                        
        population.geno_index.push_back(1);                                                                           
        break;                                                                                                        
      }
      if(genotype == "1|1"){
	population.ngeno += 1;
	population.nhoma += 1;
	population.nalt  += 2;
	population.geno_index.push_back(2);
	break;
      }
      cerr << "FATAL: unknown genotype" << endl;
      exit(1);
    }
  }

  if(population.nalt == 0 && population.nref == 0){
    population.af = -1;
  }
  else{
   
    population.af  = (population.nalt / (population.nref + population.nalt));

    if(population.nhet > 0){
      population.fis = ( 1 - ((population.nhet/population.ngeno) / (2*population.af*(1 - population.af))));
    }
    else{
      population.fis = 1;
    }
    if(population.fis < 0){
      population.fis = 0.00001;
    }
  }  
}

void loadIndices(map<int, int> & index, string set){
  
  vector<string>  indviduals = split(set, ",");

  vector<string>::iterator it = indviduals.begin();
  
  for(; it != indviduals.end(); it++){
    index[ atoi( (*it).c_str() ) ] = 1;
  }
}

double EHH(string haplotypes[][2], int nhaps){

  map<string , int> hapcounts;

  for(int i = 0; i < nhaps; i++){
    hapcounts[ haplotypes[i][0] ]++;
    hapcounts[ haplotypes[i][1] ]++;
  }

  
  double sum = 0;
  double nh  = 0;

  for( map<string, int>::iterator it = hapcounts.begin(); it != hapcounts.end(); it++){
    nh  += it->second; 
    sum += r8_choose(it->second, 2);
  }

  double max = (sum /  r8_choose(nh, 2));
  
  return max;

}

void printHaplotypes(string haps[][2], vector<int> target, vector<long int> pos){
  for(int snp = 0; snp < haps[0][1].length(); snp++){
    cout << pos[snp] << "\t" ;
    for(int ind = 0; ind < target.size(); ind++){
      cout << haps[target[ind]][0].substr(snp , 1) << "\t";
      cout << haps[target[ind]][1].substr(snp , 1) << "\t";
    }
    cout << endl;
  }
}

void loadImprovement(string tmpHaplotypes[][2], string haplotypes[][2], int ntarget){

  for(int i= 0; i < ntarget; i++){
    haplotypes[i][0] = tmpHaplotypes[i][0];
    haplotypes[i][1] = tmpHaplotypes[i][1];
  }

}

void appendHaplotypes(string tmpHaplotypes[][2], string haplotypes[][2], int ntarget){
  for(int i= 0; i < ntarget; i++){
    haplotypes[i][0].append(tmpHaplotypes[i][0].substr(5,20));
    haplotypes[i][1].append(tmpHaplotypes[i][1].substr(5,20));
  }

}


void loadPhased(string haplotypes[][2], list<pop> & window, int ntarget){
  for(list<pop>::iterator pos = window.begin(); pos != window.end(); pos++){                                       
    int indIndex = 0;                                                                                              
    for(vector<int>::iterator ind = pos->geno_index.begin(); ind != pos->geno_index.end(); ind++){
      string g = pos->genotypes[indIndex];
      vector< string > gs = split(g, "|");
      haplotypes[indIndex][0].append(gs[0]);
      haplotypes[indIndex][1].append(gs[1]);
      indIndex++;
    }
  }
}

void localPhase(string haplotypes[][2], list<pop> & window, int ntarget){
 
  double ehhmax = 0;
  
  string totalHaplotypes[ntarget][2];
  
  for(int k = 0; k < 1000; k++){
    
    string tempHaplotypes[ntarget][2] ;
    
    int tlength = haplotypes[0][0].size();
    
    for(int nt = 0; nt < ntarget ; nt++){
      if(tlength > 0 ){
   	tempHaplotypes[nt][0] = haplotypes[nt][0].substr(tlength - 5, 5);
   	tempHaplotypes[nt][1] = haplotypes[nt][1].substr(tlength - 5, 5);
      }
      else{
   	tempHaplotypes[nt][0] = "00000";
   	tempHaplotypes[nt][1] = "00000";
      }
    }
 
    int snpIndex  = 0;

    for(list<pop>::iterator pos = window.begin(); pos != window.end(); pos++){    
      
      int indIndex = 0;

      for(vector<int>::iterator ind = pos->geno_index.begin(); ind != pos->geno_index.end(); ind++){      
	int g = pos->geno_index[indIndex];
	double rang  = ((double)rand() / (double)(RAND_MAX));
	
	
	if(g == -1){
	  g = rand() % 3;
	}
	if(g == 0){
	  tempHaplotypes[indIndex][0].append("0");    
	  tempHaplotypes[indIndex][1].append("0");    
	}
	if(g == 1){
	  if(rang < 0.5){                        
	    tempHaplotypes[indIndex][0].append("0");
	    tempHaplotypes[indIndex][1].append("1");  
	  }                                                                                                                           	  else{                                                                  
	    tempHaplotypes[indIndex][0].append("1");                                    
	    tempHaplotypes[indIndex][1].append("0"); 
	  }
	}
	if(g == 2){
	  tempHaplotypes[indIndex][0].append("1");                                    
	  tempHaplotypes[indIndex][1].append("1");                                    
	}

//if(rang < pos->unphred_p[indIndex][0]){
//  g = 0;
//}
//else if(rang < pos->unphred_p[indIndex][1]){
//  g = 1;
//}
//else{
//  g = 2;
//}	
//if(g == 0 ){
//  tempHaplotypes[indIndex][0].append("0");
//  tempHaplotypes[indIndex][1].append("0");
//}
//if( g == 2 ){	
//  tempHaplotypes[indIndex][0].append("1");
//  tempHaplotypes[indIndex][1].append("1");
//}
//if(g == 1){
//  double ranh  = ((double)rand() / (double)(RAND_MAX)); 
//  if(ranh < 0.5){
//    tempHaplotypes[indIndex][0].append("0");
//    tempHaplotypes[indIndex][1].append("1");
//  }
//  else{
//    tempHaplotypes[indIndex][0].append("1");
//    tempHaplotypes[indIndex][1].append("0");
//  }
//}	
	indIndex++; 
      }
      snpIndex++; 
    } 
    double ehh = EHH(tempHaplotypes, ntarget);
    if(ehh > ehhmax){
      ehhmax = ehh;
      loadImprovement(tempHaplotypes, totalHaplotypes, ntarget);
    }
  }
  appendHaplotypes(totalHaplotypes, haplotypes, ntarget);
}

  

int main(int argc, char** argv) {

  // set the random seed for MCMC

  srand((unsigned)time(NULL));

  // the filename

  string filename = "NA";

  // set region to scaffold

  string region = "NA"; 

  // using vcflib; thanks to Erik Garrison 

  VariantCallFile variantFile;

  // zero based index for the target and background indivudals 
  
  map<int, int> it, ib;
  
  // deltaaf is the difference of allele frequency we bother to look at 

  string deltaaf ;
  double daf  = 0;

  // ancestral state is set to zero by default

  string mut = "1";

  int counts = 0;

  int phased = 0;

    const struct option longopts[] = 
      {
	{"version"   , 0, 0, 'v'},
	{"help"      , 0, 0, 'h'},
        {"file"      , 1, 0, 'f'},
	{"target"    , 1, 0, 't'},
	{"background", 1, 0, 'b'},
	{"deltaaf"   , 1, 0, 'd'},
	{"region"    , 1, 0, 'r'},
	{"mutation"  , 1, 0, 'm'},
	{"phased"    , 1, 0, 'p'},
	{0,0,0,0}
      };

    int findex;
    int iarg=0;

    while(iarg != -1)
      {
	iarg = getopt_long(argc, argv, "m:r:d:t:b:f:hv", longopts, &findex);
	
	switch (iarg)
	  {
	  case 'h':
	    cerr << endl << endl;
	    cerr << "INFO: help" << endl;
	    cerr << "INFO: description:" << endl;

	    cerr << "INFO: plotHapps  --target 0,1,2,3,4,5,6,7 --background 11,12,13,16,17,19,22 --file my.vcf --deltaaf 0.1 --ancestral 0        " << endl;
	    cerr << endl;
	    cerr << "INFO: required: r,region     -- a genomice range to calculate gl-XPEHH on in the format : seqid:start-end                   " << endl;
	    cerr << "INFO: required: t,target     -- a zero bases comma seperated list of target individuals corrisponding to VCF columns        " << endl;
	    cerr << "INFO: required: b,background -- a zero bases comma seperated list of background individuals corrisponding to VCF columns    " << endl;
	    cerr << "INFO: required: f,file a     -- proper formatted VCF.  the FORMAT field MUST contain \"PL\"                                 " << endl; 
	    cerr << "INFO: optional: m,mutation   -- which state is derived in vcf [0,1] default is 1                                            " << endl;

	    cerr << endl; 
	    cerr << "INFO: version 1.0.0 ; date: April 2014 ; author: Zev Kronenberg; email : zev.kronenberg@utah.edu " << endl;
	    cerr << endl << endl;
	    return 0;

	  case 'p':
	    phased = atoi(optarg);
	    cerr << "INFO: setting phase to:" << phased << endl;
	    break;
	  case 'v':
	    cerr << endl << endl;
	    cerr << "INFO: version 1.0.0 ; date: April 2014 ; author: Zev Kronenberg; email : zev.kronenberg@utah.edu "  << endl;
	    return 0;
	  case 'm':
	    mut = optarg;
	    cerr << "INFO derived state set to " << mut << endl;
	    break;
	  case 't':
	    loadIndices(it, optarg);
	    cerr << "INFO: there are " << it.size() << " individuals in the target" << endl;
	    cerr << "INFO: target ids: " << optarg << endl;
	    break;
	  case 'b':
	    loadIndices(ib, optarg);
	    cerr << "INFO: there are " << ib.size() << " individuals in the background" << endl;
	    cerr << "INFO: background ids: " << optarg << endl;
	    break;
	  case 'f':
	    cerr << "INFO: file: " << optarg  <<  endl;
	    filename = optarg;
	    break;
	  case 'd':
	    cerr << "INFO: only scoring sites where the allele frequency difference is greater than: " << optarg << endl;
	    deltaaf = optarg;
	    daf = atof(deltaaf.c_str());	    
	    break;
	  case 'r':
            cerr << "INFO: set seqid region to : " << optarg << endl;
	    region = optarg; 
	    break;
	  default:
	    break;
	  }

      }

    if(filename == "NA"){
      cerr << "FATAL: did not specify a file" << endl;
      cerr << "INFO: please use gl-XPEHH --help" << endl;
      return(1);
    }


    variantFile.open(filename);
    

   if(region != "NA"){
     variantFile.setRegion(region); 
   }
    
    if (!variantFile.is_open()) {
        return 1;
    }
    
    Variant var(variantFile);

    vector<string> samples = variantFile.sampleNames;
    vector<int>    target_h, background_h;

    int index, indexi = 0;

    cerr << "INFO: there are " << samples.size() << " individuals in the VCF" << endl;

    if(samples.size() == 0){
      cerr << "FATAL: too few samples or no VCF header"    << endl;
      cerr << "INFO: please use gl-XPEHH --help"           << endl;
      return(1);
    }

    for(vector<string>::iterator samp = samples.begin(); samp != samples.end(); samp++){
     
      if(it.find(index) != it.end() ){
	target_h.push_back(indexi);
	indexi++;
      }
      if(ib.find(index) != ib.end()){
	background_h.push_back(indexi);
	indexi++;
      }
      index++;
    }
    

    list< pop > tdat, bdat, zdat;

    vector<long int> positions;

    string haplotypes [it.size() + ib.size()][2];    

    while (variantFile.getNextVariant(var)) {
        map<string, map<string, vector<string> > >::iterator s     = var.samples.begin(); 
        map<string, map<string, vector<string> > >::iterator sEnd  = var.samples.end();
        
	// biallelic sites naturally 

	if(var.alt.size() > 1){
	  continue;
	}

	vector < map< string, vector<string> > > target, background, total;
	        
	int sindex = 0;

        for (; s != sEnd; s++) {	  
	  
	  map<string, vector<string> >& sample = s->second;
	  
	  if(it.find(sindex) != it.end() ){
	    target.push_back(sample);
	    total.push_back(sample);	
	  }
	  if(ib.find(sindex) != ib.end()){
	    background.push_back(sample);
	    total.push_back(sample);
	  }  
	  sindex += 1;
	}
	
	pop popt, popb, popz;

	initPop(popt);
	initPop(popb);
	initPop(popz);

	loadPop(target,     popt, var.sequenceName, var.position, phased );
	loadPop(background, popb, var.sequenceName, var.position, phased );
	loadPop(total,      popz, var.sequenceName, var.position, phased );

	
	tdat.push_back(popt);
	bdat.push_back(popb);
	zdat.push_back(popz);

	cerr << var.position << endl;
       
	positions.push_back(var.position);
	
	while(zdat.size() >= 20 && !zdat.empty()){
	  if(phased == 0){
	    localPhase(haplotypes, zdat, (it.size() + ib.size()));
	  }
	  else{
	    loadPhased(haplotypes, zdat, (it.size() + ib.size()));
	  }
	  while(!zdat.empty()){
	    zdat.pop_front();
	  }
	}
    }

    if(phased == 0){
      localPhase(haplotypes, zdat, (it.size() + ib.size()));
    }
    else{
      loadPhased(haplotypes, zdat, (it.size() + ib.size()));
    }
    while(!zdat.empty()){
      zdat.pop_front();
    }
  
    cerr << "INFO: phasing done" << endl;
   
    printHaplotypes( haplotypes, target_h, positions);

    cerr << "INFO: plotHaps finished" << endl;

    return 0;		    
}
