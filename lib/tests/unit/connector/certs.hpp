#include <string>

std::string getCaPath();
std::string getCertPath();
std::string getKeyPath();
std::string getMismatchedKeyPath();
std::string getProtectedKeyPath();
std::string getNotACertPath();
std::string getNotExistentFilePath();

// The original set of certs are not reproducable
// The ssl-new directory contains a shell script that
// allows certs to be regenerated and new certs to easily
// be generated. These getters point to the new ssl data.
std::string getCaPathCrl();
std::string getGoodCertPathCrl();
std::string getGoodKeyPathCrl();
std::string getBadCertPathCrl();
std::string getBadKeyPathCrl();
std::string getEmptyCrlPath();
std::string getRevokedCrlPath();
