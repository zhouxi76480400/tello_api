#include <iostream>
#include "tello_api/tello.h"

int main() {
    Tello * tello = new Tello();

    delete(tello);
    return 0;
}