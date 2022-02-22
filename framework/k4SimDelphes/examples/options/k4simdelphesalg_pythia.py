from Gaudi.Configuration import *
from GaudiKernel import SystemOfUnits as units

from Configurables import ApplicationMgr
app = ApplicationMgr()
app.EvtMax = 100
app.EvtSel = "NONE"

from Configurables import k4DataSvc
podioevent = k4DataSvc("EventDataSvc")
app.ExtSvc += [podioevent]


from Configurables import PythiaInterface
pythia8gentool = PythiaInterface()
### Example of pythia configuration file to generate events
# take from $K4GEN if defined, locally if not
path_to_pythiafile = os.environ.get("K4GEN", "")
pythiafilename = "Pythia_standard.cmd"
pythiafile = os.path.join(path_to_pythiafile, pythiafilename)
# Example of pythia configuration file to read LH event file
#pythiafile="options/Pythia_LHEinput.cmd"
pythia8gentool.pythiacard = pythiafile
pythia8gentool.doEvtGenDecays = False
pythia8gentool.printPythiaStatistics = True

from Configurables import GenAlg
gun = GenAlg()
gun.SignalProvider = pythia8gentool
gun.hepmc.Path = "hepmc"
ApplicationMgr().TopAlg += [gun]


from Configurables import HepMCToEDMConverter
hepmc_converter = HepMCToEDMConverter()
hepmc_converter.hepmc.Path="hepmc"
hepmc_converter.GenParticles.Path = "GenParticles"
ApplicationMgr().TopAlg += [hepmc_converter]

from Configurables import k4SimDelphesAlg
delphesalg = k4SimDelphesAlg()
delphesalg.DelphesCard = "delphes_card_IDEA.tcl"
delphesalg.DelphesOutputSettings = "edm4hep_output_config.tcl"
delphesalg.GenParticles.Path = "GenParticles"
delphesalg.OutputLevel = VERBOSE
ApplicationMgr().TopAlg += [delphesalg]


from Configurables import PodioOutput
out = PodioOutput("out", filename = "output_k4SimDelphes_pythia.root")
out.outputCommands = ["keep *"]
ApplicationMgr().TopAlg += [out]


