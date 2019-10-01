AmberChain
==========

AmberChain is the blockchain implementation for the [AmberTime](https://ambertime.org/) platform. AmberTime blockchain is focused on providing a platform for education + travel applications.

AmberChain is based on the popular open source [MultiChain](http://www.multichain.com/), customized with a host of innovative features including new API calls, support for an ecommerce marketplace, automated escrow management, and additional transaction fee parameters.

    Copyright (c) 2018 Apsaras Group Ltd
    License: GNU General Public License version 3, see COPYING

    Portions copyright (c) 2014-2017 Coin Sciences Ltd
    Portions copyright (c) 2009-2016 The Bitcoin Core developers
    Portions copyright many others - see individual files

## Documentation

Details on the API calls developed for AmberTime Blockhain can be found [here](https://github.com/ambertime/amberchain/blob/amber-dev/docs/AmberTime%20Blockchain%20Documentation_v1.4.pdf).

## Setup instructions

Refer to the original multichain instructions [here](multichain-README.md)

-----------------------------

### Linux Build Notes (on Ubuntu 14.04 x64)


1. Clone the AmberChain repository on your local directory
    ```python
    # if git is not installed
    sudo apt-get install git
    git clone https://github.com/ambertime/amberchain.git
    ```

2.  Install the required tools and libraries
    ```python
    sudo apt-get update
    sudo apt-get install build-essential libtool autotools-dev automake pkg-config libssl-dev libevent-dev bsdmainutils
    sudo apt-get install libboost-all-dev
    sudo apt-get install software-properties-common
    sudo add-apt-repository ppa:bitcoin/bitcoin
    sudo apt-get update
    sudo apt-get install libdb4.8-dev libdb4.8++-dev
    ```

3. Build the Chain 
    
    Compile Amberchain for Ubuntu (64-bit)
    ```python
    # amberchain/src >
    ./autogen.sh
    ./configure
    make
    # this will take a few minutes  to build the chain
    ```
* This will build `amberchaind`, `amberchain-cli` and `amberchain-util` in the `src` directory.

* The release is built with GCC after which `strip amberchaind` strings the debug symbols, which reduces the executable size by about 90%.

4. Connect to the AmberChain testnet or AmberChain mainnet

    **To connect to the currently deployed amber-testnet chain, run any of the following:*
    *   `./amberchaind amber-testnet@13.250.176.149:7362`
    *   `./amberchaind amber-testnet@13.124.35.92:7362`
    *   `./amberchaind amber-testnet@47.75.251.94:7362`

    **To connect to the currently deployed ambertime (mainnet) chain, run any of the following:*
    *   `./amberchaind ambertime@13.251.21.213:7362`
    *   `./amberchaind ambertime@52.220.101.207:7362`
    *   `./amberchaind ambertime@13.114.69.248:7362`
    *   `./amberchaind ambertime@52.78.32.33:7362`
    *   `./amberchaind ambertime@52.79.183.164:7362`
    *   `./amberchaind ambertime@47.88.220.28:7362`
    *   `./amberchaind ambertime@47.244.33.129:7362`
    *   `./amberchaind ambertime@47.91.17.246:7362`
    *   `./amberchaind ambertime@47.74.181.39:7362`
    *   `./amberchaind ambertime@149.129.223.190:7362`
    *   `./amberchaind ambertime@137.116.137.133:7362`
    *   `./amberchaind ambertime@40.115.176.120:7362`
    *   `./amberchaind ambertime@52.231.158.148:7362`
    *   `./amberchaind ambertime@13.76.197.197:7362`
    *   `./amberchaind ambertime@52.231.76.73:7362`

5. Run the Chain CLI
    ```python
    # amberchain/src >
    ./amberchain-cli <chain name>
    # ie: ./amberchain-cli amber-testchain

6. Stop the Chain

    * You can stop the chain by running the stop command from outside the CLI
    ```python
    # amberchain/src >
    ./amberchain-cli <chain name> stop
    # ie. ./amberchain-cli amber-testchain stop
    ```
    * Inside the CLI 
    ```python
    # amber-testchain >
    stop
    bye
    ```

### Deploy the Cron Job

* To automatically restart the `ambertime` chain, follow the steps below.
* If setting this up for `amber-testnet`, please replace `ambertime` to `amber-testnet` in `restart_chain.sh`.
* `check_chain.sh` assumes that there is only one amberchaind process running on your machine.
* These steps assume that the `amberchain` directory is under your `$HOME|~` folder.

1. Create the `cron_logs` directory under `$HOME`
   ```
   mkdir ~/cron_logs
   ```
2. Install the cron job
   * Run command below
   ```
   crontab -e
   ```
   * Append your entry at the end of the file
   * (Note that the command below runs the `check_chain.sh` script every 5 minutes)
   ```
   */5 * * * * $HOME/amberchain/check_chain.sh
   ```
