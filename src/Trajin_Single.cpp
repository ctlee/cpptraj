#include "Trajin_Single.h"
#include "CpptrajStdio.h"

// CONSTRUCTOR
Trajin_Single::Trajin_Single() :
  trajio_(0),
  trajIsOpen_(false)
{}

// DESTRUCTOR
Trajin_Single::~Trajin_Single() {
  if (trajio_!=0) {
    if (trajIsOpen_) EndTraj();
    delete trajio_;
  }
}

// Trajin_Single::SetupTrajRead()
int Trajin_Single::SetupTrajRead(std::string const& tnameIn, ArgList *argIn, Topology *tparmIn) 
{
  // Require a filename
  if (tnameIn.empty()) {
    mprinterr("Internal Error: Trajin_Single: No filename given.\n");
    return 1;
  }
  // Check and set associated parm file
  if ( SetTrajParm( tparmIn ) ) return 1;
  // Check that file exists
  if (!fileExists(tnameIn.c_str())) {
    mprinterr("Error: File %s does not exist.\n",tnameIn.c_str());
    return 1;
  }
  // Set up CpptrajFile
  CpptrajFile baseFile;
  if (baseFile.SetupRead( tnameIn, debug_ )) return 1;
  // Set file name and base file name
  SetFileNames( tnameIn, baseFile.BaseFileName() );
  // Detect format
  if ( (trajio_ = DetectFormat( baseFile )) == 0 ) {
    mprinterr("Error: Could not set up file %s for reading.\n", tnameIn.c_str());
    return 1;
  }
  // Set up the format for reading and get the number of frames.
  if (SetupTrajIO( trajio_, argIn )) return 1;
  // Check how many frames will actually be read
  if (setupFrameInfo() == 0) return 1;
  // If the trajectory has box coords, set the box type from the box Angles.
  if (trajio_->CheckBoxInfo(TrajParm())!=0) {
    mprinterr("Error in trajectory %s box information.\n",BaseTrajStr());
    return 1;
  }
  return 0;
}

// Trajin_Single::BeginTraj()
int Trajin_Single::BeginTraj(bool showProgress) {
  // Open the trajectory
  if (trajio_->openTraj()) {
    mprinterr("Error: Trajin_Single::BeginTraj: Could not open %s\n",BaseTrajStr());
    return 1;
  }
  // Set progress bar, start and offset.
  PrepareForRead( showProgress, trajio_->Seekable() );
  trajIsOpen_ = true;
  return 0;
}

// Trajin_Single::EndTraj()
void Trajin_Single::EndTraj() {
  if (trajIsOpen_) {
    trajio_->closeTraj();
    trajIsOpen_ = false;
  }
}

// Trajin_Single::GetNextFrame()
int Trajin_Single::GetNextFrame( Frame& frameIn ) {
  // If the current frame is out of range, exit
  if ( CheckFinished() ) return 0;

  bool tgtFrameFound = false;
  while ( !tgtFrameFound ) {
    if (trajio_->readFrame(CurrentFrame(), frameIn.xAddress(), frameIn.vAddress(),
                           frameIn.bAddress(), frameIn.tAddress()))
      return 0;
    //printf("DEBUG:\t%s:  current=%i  target=%i\n",trajName,currentFrame,targetSet);
    tgtFrameFound = ProcessFrame();
  }

  return 1;
}

// Trajin_Single::PrintInfo()
void Trajin_Single::PrintInfo(int showExtended) {
  mprintf("  [%s] ",BaseTrajStr());
  trajio_->info();
  mprintf(", Parm %s",TrajParm()->c_str());
  if (trajio_->HasBox()) mprintf(" (with box info)");
  if (showExtended==1) PrintFrameInfo(); 
  if (debug_>0)
    mprintf(", %i atoms, Box %i, seekable %i",TrajParm()->Natom(),(int)trajio_->HasBox(),
            (int)trajio_->Seekable());
  mprintf("\n");
}

// Trajin_Single::HasVelocity()
bool Trajin_Single::HasVelocity() {
  if (trajio_!=0) return trajio_->HasVelocity();
  return false;
}
