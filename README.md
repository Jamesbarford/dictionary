# A self updating dictionary

The dictionary can run as a background daemon and is used through `./define`. The dictionary is cached in a hashtable for speed and persisted to a `sqlite` database. If a definition does not exist in the database a request is made to merriam webster, then html parsed and the relivant parts stored in the cache then persisted to the database.

## Usage:

```bash
# to start the dameon
./dict-server

# to search a word (case insensative)

define <string>
```

## Example
```sh
# start up the server
./dict-server

# search for word
./define cat

# output
1) a carnivorous mammal (Felis catus) long domesticated as a pet and for catching rats and mice
2) any of a family (Felidae) of carnivorous usually solitary and nocturnal mammals (such as the domestic cat, lion, tiger, leopard, jaguar, cougar, wildcat, lynx, and cheetah)
3) guy
4) a player or devotee of jazz
5) a strong tackle used to hoist an anchor to the cathead of a ship
6) catboat
7) catamaran
8) cat-o-nine-tails
9) catfish
```

## Dependencies

### Mac
```sh
brew install libcurl openssl sqlite
```

### Fedora
```sh
sudo dnf install openssl openssl-devel libcurl sqlite-devel
```

### Ubuntu
```sh
sudo apt-get install openssl openssl-dev libcurl-dev libsqlite3-dev
```

## Install
```sh
mkdir build

make

make install
```
