# How to run the unit test comparing converted DelphesEDM4HEP output to direct Delphes output with various Delphes cards

## Setting up

Check out this fork of the repo and compile: 

```
source /cvmfs/sw-nightlies.hsf.org/key4hep/setup.sh
git clone https://github.com/bistapf/k4SimDelphes.git
cd k4SimDelphes
mkdir build && cd build
cmake .. -DCMAKE_INSTALL_PREFIX=../install
make install
```

Everytime you edit a test, you need to recompile, i.e. execute the last two steps. To run the tests, stay in the `build` directory.

## Running the tests
To run any test, make sure you are in the `build` directory. 

### FCC-ee events with IDEA card

This is the unit test that comes with the release and is passed, you can run it like this: 
```ctest -R PythiaConverter_ee_Z_bbbar --verbose``` 

It generates 1000 ee-events on the fly, and compares direct Delphes vs EDM4HEP output. The card it uses is found here: [`delphes_card_IDEA.tcl`](https://github.com/delphes/delphes/blob/master/cards/delphes_card_IDEA.tcl). 

### FCC-hh events and cards

I added two tests to run with FCC-hh events and the old and new FCC-hh card:

```ctest -R PythiaConverter_ggHH_100TeV_FCCCard --verbose``` uses the new card at [`/FCC/scenarios/FCChh_I.tcl`](https://github.com/delphes/delphes/blob/master/cards/FCC/scenarios/FCChh_I.tcl)

```ctest -R PythiaConverter_ggHH_100TeV_oldFCChhCard --verbose``` uses the old card at [`/FCC/FCChh.tcl`](https://github.com/delphes/delphes/blob/master/cards/FCC/FCChh.tcl)

Both of these test will fail with something like: 
``` 6: EVENT: 1
6: Delphes and edm4hep candidate 11 in collection 'EFlowTrack' have different kinematics: (0, 0, -nan, -nan) vs (0,0,-nan,-nan)
```

The same happens also for using the ee-event generation with the new FCC-hh Delphes card, so the issue seems to be the card, not the events. 

### Checking the test output files manually

The tests each produce two output files, one is the direct Delphes output the other the DelphesEDM4HEP output. You will find them in `build/tests/` and they will be named 
`<name_of_the_test>_delphes.root` and `<name_of_the_test.root`. The edm4hep output is not so easy to browse interactively, so you can use the script `check_testoutput.py` to read it and check for `nan` values. Currently it runs on one output file of the bbWW signal generated with `EventProducer`, also finding `nan`s, so the problem is not related to the test setup. 
