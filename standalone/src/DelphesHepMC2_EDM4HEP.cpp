
#include "DelphesHepMC2Reader.h"
#include "DelphesMain.h"

int main(int argc, char* argv[]) {
  DelphesHepMC2InputReader inputReader = DelphesHepMC2InputReader();
  return doit(argc, argv, inputReader);
}
