from Gaudi.Configuration import *
from GaudiKernel import SystemOfUnits as units

from Configurables import ApplicationMgr
app = ApplicationMgr()
app.EvtMax = 3
app.EvtSel = "NONE"

from Configurables import k4DataSvc
podioevent = k4DataSvc("EventDataSvc")
app.ExtSvc += [podioevent]

from Configurables import ConstPtParticleGun
guntool1 = ConstPtParticleGun(PdgCodes=[-211], PtMin=50*units.GeV, PtMax=50*units.GeV)

from Configurables import GenAlg
gun = GenAlg()
gun.SignalProvider = guntool1
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
out = PodioOutput("out", filename = "output_k4SimDelphes.root")
out.outputCommands = ["keep *"]
ApplicationMgr().TopAlg += [out]


