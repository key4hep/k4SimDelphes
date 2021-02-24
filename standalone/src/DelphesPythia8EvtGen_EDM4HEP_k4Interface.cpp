
#include "DelphesMain.h"
#include "DelphesPythia8EvtGenReader_k4Interface.h"

int main(int argc, char *argv[]) {
  DelphesPythia8EvtGenReader_k4Interface inputReader = DelphesPythia8EvtGenReader_k4Interface();
  return doit(argc, argv, inputReader);
}
