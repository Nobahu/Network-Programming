#include "server.h"

int main()
{
  SelectServer server(5050);
  server.run();

  return 0;
}