sudo apt-get install build-essential zlib1g-dev
make
cc -o setupwizard setupwizard.c
cc -o adduser adduser.c
./setupwizard
./adduser
chmod +x alpha-start