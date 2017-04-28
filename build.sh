cd 
sudo apt-get install git build-essential zlib1g-dev
git clone https://github.com/huenato/AlphaRO.git
cd ./AlphaRO
make
cc -o setupwizard setupwizard.c
cc -o adduser adduser.c
./setupwizard
./adduser
chmod +x alpha-start