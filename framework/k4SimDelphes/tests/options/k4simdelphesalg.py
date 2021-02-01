from Gaudi.Configuration import *

PluginDebugLevel=1
from Configurables import ApplicationMgr
app = ApplicationMgr()
app.EvtMax = 100
app.EvtSel = "NONE"

from Configurables import k4DataSvc
podioevent = k4DataSvc("EventDataSvc")
app.ExtSvc += [podioevent]

from Configurables import ConstPtParticleGun
gun = ConstPtParticleGun()
app.TopAlg = [gun]

#from Configurables import k4SimDelphesAlg
#delphesalg = k4SimDelphesAlg()
#delphesalg.GenParticles.Path = "MCParticles"



