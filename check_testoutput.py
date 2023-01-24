import ROOT
from ROOT import edm4hep
import numpy as np
import os
from os import listdir
from edm4hep_path import get_edm4hep_path
from podio.root_io import Reader
import podio

ROOT.gInterpreter.LoadFile(get_edm4hep_path()+"/include/edm4hep/utils/kinematics.h")
USE_ENERGY=edm4hep.utils.detail.UseEnergyTag()

def get_tlv(particle):
	tlv = ROOT.TLorentzVector()
	tlv.SetPxPyPzE(particle.getMomentum()[0], particle.getMomentum()[1], particle.getMomentum()[2], particle.getEnergy())
	return tlv

if __name__ == "__main__":

	# edm4hep_test_file = "/eos/experiment/fcc/hh/generation/DelphesEvents/fcc_v04/pwp8_pp_hh_lambda100_5f_hhbbww/events_000096236.root" #bbWW with old production
	edm4hep_test_file = "/eos/experiment/fcc/hh/generation/DelphesEvents/fcc_v05_scenarioI/pwp8_pp_hh_lambda100_5f_hhbbww/events_000025742.root" #bbww producion with NEW CARD
	# edm4hep_test_file = "/afs/cern.ch/user/b/bistapf/Dev_k4SimDelphes/k4SimDelphes/build/tests/pythia_converter_output_ggHH_100TeV.root" #test output
# 
	reader = Reader(edm4hep_test_file) #works only with new production

	for i, event in enumerate(reader.get('events')):
		print("Processing event:", i)

		eflow_tracks = event.get("EFlowTrack")
		rp_particles = event.get("ReconstructedParticles")

		for i_rp, rp in enumerate(rp_particles):
			rp_tracks = rp.getTracks()
			if len(rp_tracks) > 0:
				tlv_rp_track = get_tlv(rp)

				# check if any nans
				if ( np.isnan(abs(tlv_rp_track.Px())) or np.isnan(abs(tlv_rp_track.Py())) or np.isnan(abs(tlv_rp_track.Pz())) or np.isnan(abs(tlv_rp_track.E())) ):
					print("Found nans for object", i_rp, "has 4 vector :", tlv_rp_track.Px(), tlv_rp_track.Py(), tlv_rp_track.Pz(), tlv_rp_track.E() )

