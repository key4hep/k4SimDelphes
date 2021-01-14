from Gaudi.Configuration import *

from Configurables import k4DataSvc
podioevent = k4DataSvc("EventDataSvc")

from Configurables import CreateExampleEventData
producer = CreateExampleEventData()

from Configurables import k4SimDelphesAlg
delphesalg = k4SimDelphesAlg()
delphesalg.GenParticles.Path = "MCParticles"


from Configurables import ApplicationMgr
app = ApplicationMgr()
app.EvtMax = 100
app.ExtSvc = [podioevent]
app.EvtSel = "NONE"
app.TopAlg = [producer, delphesalg]
