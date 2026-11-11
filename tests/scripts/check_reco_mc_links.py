#!/usr/bin/env python3
"""Check that the reco-MC links in an EDM4hep file produced by k4SimDelphes are
filled: (almost) every ReconstructedParticle has at least one link, and all
links point to particles in the given MCParticle collection.

A small fraction of reco particles legitimately has no link (e.g. zero energy
photons without a generator particle, ~1e-5 in e+e- -> Z events), hence only a
minimum linked fraction is required.

Usage: check_reco_mc_links.py <edm4hep_file> [mc_collection] [link_collection]
"""

import sys

from podio import reading


def fail(msg):
    print(f"check_reco_mc_links: FAILURE: {msg}")
    sys.exit(1)


filename = sys.argv[1]
mc_name = sys.argv[2] if len(sys.argv) > 2 else "GenParticles"
link_name = sys.argv[3] if len(sys.argv) > 3 else "RecoMCLink"
min_linked_fraction = 0.99

n_events = 0
n_reco = 0
n_links = 0
n_linked_reco = 0
for frame in reading.get_reader(filename).get("events"):
    reco = frame.get("ReconstructedParticles")
    links = frame.get(link_name)
    mc_coll_id = frame.get(mc_name).getID()

    linked_reco = set()
    for link in links:
        if link.getTo().getObjectID().collectionID != mc_coll_id:
            fail(f"event {n_events}: link points to a particle outside of '{mc_name}'")
        linked_reco.add(link.getFrom().getObjectID().index)

    n_events += 1
    n_reco += len(reco)
    n_links += len(links)
    n_linked_reco += len(linked_reco)

if n_events == 0:
    fail("no events found")
if n_reco == 0:
    fail("no ReconstructedParticles found, cannot check links")
if n_linked_reco < min_linked_fraction * n_reco:
    fail(f"only {n_linked_reco} of {n_reco} ReconstructedParticles have a link")

print(f"check_reco_mc_links: OK: {n_events} events, {n_reco} ReconstructedParticles ({n_linked_reco} linked), {n_links} links")
