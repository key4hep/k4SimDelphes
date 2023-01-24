import os 

def get_edm4hep_path():
    """
    Get the EDM4hep path from LD_LIBRARY_PATH
    NOTE: not the most elegant, nor the most robust solution, but should in general 
    work with Key4hep environments
    """
    edm4hep_lib_path = [
        p for p in os.environ["LD_LIBRARY_PATH"].split(":") if "/edm4hep/" in p
    ][0]
    edm4hep_path = "/".join(edm4hep_lib_path.split("/")[:-1])
    return edm4hep_path
