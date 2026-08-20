#!/usr/bin/env python3
"""Check that an EDM4hep file contains generator event weights: every event
has a non-trivial EventHeader.weights vector with weights[0] equal to the
scalar EventHeader.weight, and the metadata frame holds a matching
EventWeightNames list.

Usage: check_event_weights.py <edm4hep_file>
"""

import sys

from podio import reading


def fail(msg):
    print(f"check_event_weights: FAILURE: {msg}")
    sys.exit(1)


reader = reading.get_reader(sys.argv[1])

n_events = 0
n_weights = 0
for frame in reader.get("events"):
    header = frame.get("EventHeader")[0]
    weights = list(header.getWeights())
    if len(weights) < 2:
        fail(f"event {n_events}: weights vector has size {len(weights)}, expected > 1")
    if weights[0] != header.getWeight():
        fail(f"event {n_events}: weights[0] = {weights[0]} != EventHeader.weight = {header.getWeight()}")
    n_weights = len(weights)
    n_events += 1

if n_events == 0:
    fail("no events in file")

names = []
frame = reader.get("metadata")[0]
names = list(frame.get_parameter("EventWeightNames"))
if len(names) != n_weights:
    fail(f"EventWeightNames has {len(names)} entries but the weights vector has {n_weights}")

print(f"check_event_weights: OK: {n_events} events x {n_weights} weights, "
      f"{len(names)} names in metadata, weights[0] == EventHeader.weight everywhere")
