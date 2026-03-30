
#include "DelphesHepMC3Reader.h"
#include "DelphesMain.h"

int main(int argc, char* argv[]) {
  DelphesHepMC3InputReader inputReader = DelphesHepMC3InputReader();
  return doit(argc, argv, inputReader);
}
