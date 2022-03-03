#include <string>

inline std::string getDelphesCard(const std::string& card = "delphes_card_IDEA.tcl") {
  const std::string cardDir = std::getenv("DELPHES_CARDS_DIR");
  return cardDir + "/" + card;
}
