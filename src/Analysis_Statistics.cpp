#include <cmath> // sqrt
#include "Analysis_Statistics.h"
#include "CpptrajStdio.h"

// CONSTRUCTOR
Analysis_Statistics::Analysis_Statistics() :
  shift_(0)
{}

/** analyze statistics {<name> | ALL} [shift <value>] */
int Analysis_Statistics::Setup(DataSetList *DSLin) {
  bool analyzeAll = false;
  // Ensure first two args are marked (analyze statistics).
  analyzeArgs_.MarkArg(0);
  analyzeArgs_.MarkArg(1);
  // Get keywords.
  shift_ = analyzeArgs_.getKeyDouble("shift", 0);
  filename_ = analyzeArgs_.GetStringKey("out");
  // Get dataset or all datasets
  if (analyzeArgs_.hasKey("all")) {
    analyzeAll = true;
    for (DataSetList::const_iterator ds = DSLin->begin(); ds != DSLin->end(); ++ds)
      datasets_.push_back( *ds );
  } else {
    std::string dsetname = analyzeArgs_.GetStringNext();
    if (dsetname.empty()) {
      mprinterr("Error: analyze statistics: No dataset name or 'all' specified.\n");
      return 1;
    }
    DataSet *tempds = DSLin->Get( dsetname.c_str() );
    if (tempds == NULL) {
      mprinterr("Error: analyze statistics: No dataset with name %s\n", dsetname.c_str());
      return 1;
    }
    datasets_.push_back( tempds );
  }
  if (datasets_.empty()) {
    mprinterr("Error: analyze statistics: No datasets to analyze.\n");
    return 1;
  }
  // INFO
  mprintf("    ANALYZE STATISTICS: ");
  if (analyzeAll)
    mprintf("ALL accumulated values ");
  else
    mprintf("name %s ", datasets_.back()->c_str() );
  if (shift_ != 0)
    mprintf("shift (about %.2f) is begin applied.", shift_);
  mprintf("\n");
  if (!filename_.empty())
    mprintf("\tOutput to file %s\n", filename_.c_str());

  return 0;
}

int Analysis_Statistics::Analyze() {
  if (outfile_.OpenWrite( filename_ )) return 1;
  for (std::vector<DataSet*>::iterator ds = datasets_.begin();
                                       ds != datasets_.end(); ++ds)
  {
    double average = 0;
    double stddev = 0;
    bool periodic = false;
    int totalFrames = (*ds)->Xmax() + 1;
    int Nelements = (*ds)->Size();
    if (Nelements < 1) {
      mprintf("Warning: analyze statistics: No data in dataset %s, skipping.\n",
              (*ds)->c_str());
      continue;
    }
    if (Nelements != totalFrames) {
      mprintf("Warning: analyze statistics: Set %s is missing data for some frames.\n",
              (*ds)->c_str());
      mprintf("Warning: Xmax= %i, Nelements= %i\n", totalFrames - 1, Nelements);
    }
    
    DataSet::scalarMode mode = (*ds)->ScalarMode();

    if (mode == DataSet::M_ANGLE ||
        mode == DataSet::M_TORSION ||
        mode == DataSet::M_PUCKER)
      periodic = true;

    // ----- Compute average and standard deviation ------------------
    for (int i = 0; i < totalFrames; ++i) {
      double value = (*ds)->Dval( i ) - shift_;
      if (periodic) {
        if (value > 180.0)
          value -= 360.0;
        else if (value < -180.0)
          value += 360.0;
      }
      average += value;
      stddev += (value*value);
    }
    average /= totalFrames;
    stddev /= totalFrames;
    stddev -= (average * average);
    if (stddev > 0)
      stddev = sqrt( stddev );
    else
      stddev = 0;
    average += shift_;

    // Output average/stddev
    outfile_.Printf("__________________________________________________________________\n\n");
    outfile_.Printf("STATISTICS %6s\n", (*ds)->c_str());
    // NOTE: Here in PTRAJ, Atom/Mask selections were output for M_DISTANCE.
    //       Not done in CPPTRAJ since currently this info is not stored
    //       in DataSet.
    outfile_.Printf("   AVERAGE: %8.4f (%.4f stddev)\n", average, stddev);
    outfile_.Printf("   INITIAL: %8.4f\n   FINAL:   %8.4f\n",
                    (*ds)->Dval( 0 ), (*ds)->Dval( totalFrames-1 ) );

    // More specific analysis based on MODE
    if ( mode == DataSet::M_PUCKER) 
      PuckerAnalysis( *ds, totalFrames ); 

  } // END loop over DataSets

  return 0;
}

const char Analysis_Statistics::pucker_ss[10][9] = {
  "C3'-endo", "C4'-exo ", "O4'-endo", "C1'-exo ", "C2'-endo", "C3'-exo ",
  "C4'-endo", "O4'-exo ", "C1'-endo", "C2'-exo "
};

void Analysis_Statistics::PuckerAnalysis( DataSet* ds, int totalFrames ) {
  int pucker_visits[10];
  int pucker_transitions[10][10];
  double pucker_avg[10];
  double pucker_sd[10];
  int curbin, prevbin;

  for (int j = 0; j < 10; ++j) {
    pucker_visits[j] = 0;
    pucker_avg[j] = 0.0;
    pucker_sd[j] = 0.0;
    for (int k=0;k<10;k++) 
      pucker_transitions[j][k] = 0;
  }

  // Get bin for first frame
  double firstvalue = ds->Dval(0);
  if (firstvalue < 0) firstvalue += 360;
  prevbin = firstvalue / 36;
  // Loop over all frames
  for (int i = 0; i < totalFrames; ++i) {
    double value = ds->Dval( i );
    double dval = value;
    if (dval < 0) dval += 360.0;
    curbin = dval / 36;

    ++pucker_visits[curbin];
    pucker_avg[curbin] += value;
    pucker_sd[curbin]  += (value*value);
    if (curbin != prevbin) 
      ++pucker_transitions[prevbin][curbin];
    prevbin = curbin;
  }

  if ( ds->ScalarType() == DataSet::PUCKER)
    outfile_.Printf("\n   This is marked as a nucleic acid sugar pucker phase\n");

  outfile_.Printf("\n            %s %s %s %s %s %s %s %s %s %s\n",
                  pucker_ss[0], pucker_ss[1], pucker_ss[2], pucker_ss[3], pucker_ss[4],
                  pucker_ss[5], pucker_ss[6], pucker_ss[7], pucker_ss[8], pucker_ss[9]);
  outfile_.Printf("           -------------------------------------");
  outfile_.Printf("------------------------------------------------------\n");

  for (int j=0; j < 10; j++) {
    if (pucker_visits[j] > 0) {
      pucker_avg[j] /= pucker_visits[j];
      pucker_sd[j]  /= pucker_visits[j];
      pucker_sd[j] = sqrt(pucker_sd[j] - pucker_avg[j]*pucker_avg[j]);
    }
  }

  // OUTPUT
  outfile_.Printf(" %%occupied |");
  for (int j=0; j < 10; j++) {
    if (pucker_visits[j] > 0) {
      double value = pucker_visits[j]*100.0/totalFrames;
      outfile_.Printf(" %6.1f |", value);
    } else
      outfile_.Printf("        |");
  }
  outfile_.Printf("\n");

  outfile_.Printf(" average   |");
  for (int j=0; j < 10; j++) {
    if (pucker_visits[j] > 0) {
      outfile_.Printf(" %6.1f |", pucker_avg[j]);
    } else
      outfile_.Printf("        |");
  }
  outfile_.Printf("\n");

  outfile_.Printf(" stddev    |");
  for (int j=0; j < 10; j++) {
    if (pucker_visits[j] > 1) {
      outfile_.Printf(" %6.1f |", pucker_sd[j]);
    } else
      outfile_.Printf("        |");
  }
  outfile_.Printf("\n           ----------------------------------------------------------");
  outfile_.Printf("---------------------------------\n");

  if (debug_ > 0) {
  outfile_.Printf("\nTRANSITIONS TABLE: (from/vertical to/horizontal)\n\n");
  outfile_.Printf("           %s %s %s %s %s %s %s %s %s %s\n",
                  pucker_ss[0], pucker_ss[1], pucker_ss[2], pucker_ss[3], pucker_ss[4],
                  pucker_ss[5], pucker_ss[6], pucker_ss[7], pucker_ss[8], pucker_ss[9]);
  outfile_.Printf("           ------------------------------------------");
  outfile_.Printf("-------------------------------------------------\n");
  for (int j=0; j<10; j++) {
    outfile_.Printf("  %s |", pucker_ss[j]);
    for (int k=0; k<10; k++) {
      if (pucker_transitions[j][k] > 0)
        outfile_.Printf(" %6i |", pucker_transitions[j][k]);
      else
        outfile_.Printf("        |");
    }
    outfile_.Printf("\n");
  }
  outfile_.Printf("           ----------------------------------------------------------");
  outfile_.Printf("---------------------------------\n\n");
  }
}

const char Analysis_Statistics::torsion_ss[6][8] = {
  "g+     ", "a+     ", "t      ", "a-     ", "g-     ", "c      "
};

const double Analysis_Statistics::torsion_offset[6] = { 
  0.0, 0.0, 0.0, 0.0, 0.0, 180.0 
};

void Analysis_Statistics::TorsionAnalysis(DataSet* ds, int totalFrames) {
  int torsion_visits[6];
  int torsion_transitions[6][6];
  double torsion_avg[10];
  double torsion_sd[10];
  int prevbin, curbin;

  for (int j=0;j<6;j++) {
    torsion_visits[j] = 0;
    torsion_avg[j] = 0.0;
    torsion_sd[j] = 0.0;
    for (int k=0;k<6;k++) 
      torsion_transitions[j][k] = 0;
  }
 
  // Get initial bin
  double firstvalue = ds->Dval( 0 );
  if (firstvalue < 0) firstvalue += 360;
  prevbin = (int) (firstvalue - 30.0) / 60;
  // Loop over all frames
  for (int i = 0; i < totalFrames; ++i) {
    double value = ds->Dval( i );
    double dval = value;
    if (dval < 0) dval += 360;
    curbin = (int) (dval - 30.0) / 60;

    ++torsion_visits[curbin];
    value += torsion_offset[curbin];
    // Fix for trans averaging
    if (value < -150.0) value += 360.0;
    torsion_avg[curbin] += value;
    torsion_sd[curbin]  += (value*value);
    if (curbin != prevbin) 
      ++torsion_transitions[prevbin][curbin];
    prevbin = curbin;
  }

  // OUTPUT
  outfile_.Printf("\n               %s  %s  %s  %s  %s  %s\n",
          torsion_ss[0], torsion_ss[1], torsion_ss[2],
          torsion_ss[3], torsion_ss[4], torsion_ss[5]);
  outfile_.Printf("           ---------------");
  outfile_.Printf("----------------------------------------\n");

  for (int j=0; j < 6; j++) {
    if (torsion_visits[j] > 0) {
      torsion_avg[j] /= torsion_visits[j];
      torsion_sd[j]  /= torsion_visits[j];
      torsion_sd[j] = sqrt(torsion_sd[j] - torsion_avg[j]*torsion_avg[j]);
      torsion_avg[j] -= torsion_offset[j];
    }
  }

  outfile_.Printf(" %%occupied |");
  for (int j=0; j < 6; j++) {
    if (torsion_visits[j] > 0) {
      double value = torsion_visits[j]*100.0/totalFrames;
      outfile_.Printf(" %6.1f |", value);
    } else
      outfile_.Printf("        |");
  }
  outfile_.Printf("\n");

  outfile_.Printf(" average   |");
  for (int j=0; j < 6; j++) {
    if (torsion_visits[j] > 0) {
      outfile_.Printf(" %6.1f |", torsion_avg[j]);
    } else
      outfile_.Printf("        |");
  }
  outfile_.Printf("\n");

  outfile_.Printf(" stddev    |");
  for (int j=0; j < 6; j++) {
    if (torsion_visits[j] > 1) {
      outfile_.Printf(" %6.1f |", torsion_sd[j]);
    } else
      outfile_.Printf("        |");
  }
  outfile_.Printf("\n           --------------------------");
  outfile_.Printf("-----------------------------\n");

  // Specific torsion types
  switch ( ds->ScalarType() ) {
    case DataSet::ALPHA:
      //              "               g+       a+       t        a-       g-       c 
      outfile_.Printf(" ALPHA       minor             minor            canonical\n");
      outfile_.Printf("\n   O3'-P-O5'-C5', SNB range is 270-300 deg (g-)\n");
      if ( (torsion_visits[0] + torsion_visits[1] + torsion_visits[2] + torsion_visits[5] )
           > (totalFrames * 0.1) )
        outfile_.Printf("   *** > 10%% out of range population detected\n");
      break;

    case DataSet::BETA:
      //              "               g+       a+       t        a-       g-       c 
      outfile_.Printf(" BETA                <-- canonical -->\n");

      outfile_.Printf("\n   P-O5'-C5'-C4', SNB range is 130-200 deg (a+,t)\n");
      if ( (torsion_visits[0] + torsion_visits[3] + torsion_visits[4] + torsion_visits[5] )
           > (totalFrames * 0.05) )
        outfile_.Printf("   *** > 5%% out of range population detected\n");
      break;

    case DataSet::GAMMA:
      //              "               g+       a+       t        a-       g-       c 
      outfile_.Printf(" GAMMA     canonical           minor             minor\n");
      outfile_.Printf("\n   O5'-C5'-C4'-C3', SNB range is 20-80 (g+)\n");
      if (torsion_visits[2] > (totalFrames* 0.1))
        outfile_.Printf("   *** GAMMA trans > 10%% detected!!!\n");
      break;

    case DataSet::DELTA:
      //              "               g+       a+       t        a-       g-       c 
      outfile_.Printf(" DELTA      <------ canonical ------>\n");
      outfile_.Printf("\n   C5'-C4'-C3'-O3', SNB range is 70-180\n");
      outfile_.Printf("   DNA: ~128 with BI (a+), ~144 with BII (a+)\n");
      if ( (torsion_visits[0] + torsion_visits[3] + torsion_visits[4] + torsion_visits[5] )
           > (totalFrames * 0.05) )
        outfile_.Printf("   *** > 5%% out of range population detected\n");
      break;

    case DataSet::EPSILON:
      //              "               g+       a+       t        a-       g-       c 
      outfile_.Printf(" EPSILON                         BI       BII\n");
      outfile_.Printf("\n   C4'-C3'-O3'-P, SNB range is 160-270\n");
      outfile_.Printf("   BI = %6.2f%% (~184), BII = %6.2f%% (~246)\n",
              (torsion_visits[2]*100.0)/totalFrames,
              (torsion_visits[3]*100.0)/totalFrames);
      if ( (torsion_visits[0] + torsion_visits[1] + torsion_visits[4] + torsion_visits[5] )
           > (totalFrames * 0.05) )
        outfile_.Printf("   *** > 5%% out of range population detected\n");
      break;

    case DataSet::ZETA:
      //              "               g+       a+       t        a-       g-       c 
      outfile_.Printf(" ZETA                <----- BII ------------- BI ----->\n");
      outfile_.Printf("\n   C3'-O3'-P-O5', SNB range is 130-300\n");
      outfile_.Printf("   BI = %6.2f%% (~265, a-/g-), BII = %6.2f%% (~174, a+/t)\n",
              (torsion_visits[3]+torsion_visits[4])*100.0/totalFrames,
              (torsion_visits[1]+torsion_visits[2])*100.0/totalFrames);
      if ( (torsion_visits[0] + torsion_visits[5] )
           > (totalFrames * 0.05) )
        outfile_.Printf("   *** > 5%% out of range population detected\n");
      break;

    case DataSet::CHI:
      //              "               g+       a+       t        a-       g-       c 
      outfile_.Printf(" CHI                         <-------- anti ------->  <--syn---\n");
      outfile_.Printf("\n   O4'-C1'-NX-CX, SNB range is 200-300\n");
      if ( (torsion_visits[0] + torsion_visits[5] ) > (totalFrames * 0.05) )
        outfile_.Printf(
          "   *** CHI flips; > 5%% out of range populations detected (see table below)\n");
      if ( torsion_visits[1] > (totalFrames * 0.05) )
        outfile_.Printf("   *** Unexpected CHI population in a+ region, > 5%%\n");
      break;

    case DataSet::C2P:
      //              "               g+       a+       t        a-       g-       c 
      outfile_.Printf(" C2' to base      in\n");
      outfile_.Printf("\n   C2'-C1'-NX-CX\n\n");
      break;

    case DataSet::H1P:
      //              "               g+       a+       t        a-       g-       c 
      outfile_.Printf(" H1'       below-plane                           above      in\n");
      outfile_.Printf("\n   H1'-C1'-NX-CX, > 0 H1' below plane (check if sugar in plane)\n\n");
      break;

    default: break;
  } // END switch over dataset scalartype

  if (debug_ > 0) {
    outfile_.Printf("\nTRANSITIONS TABLE: (from/vertical to/horizontal)\n\n");
    outfile_.Printf("              %s  %s  %s  %s  %s  %s\n",
            torsion_ss[0], torsion_ss[1], torsion_ss[2],
            torsion_ss[3], torsion_ss[4], torsion_ss[5]);
    outfile_.Printf("           -----------------------");
    outfile_.Printf("--------------------------------\n");
    for (int j=0; j<6; j++) {
      outfile_.Printf("   %s |", torsion_ss[j]);
      for (int k=0; k<6; k++) {
        if (torsion_transitions[j][k] > 0)
          outfile_.Printf(" %6i |", torsion_transitions[j][k]);
        else
          outfile_.Printf("        |");
      }
      outfile_.Printf("\n");
    }
    outfile_.Printf("           ------------------");
    outfile_.Printf("-------------------------------------\n\n");
  }
};