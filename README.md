# wiimote-btstack
Access data from third party remotes not recognised by the normal bluetooth stack.

Very work in progress, currently known to work with cheap black third-party Wii remote from eBay.

Does not work with official remotes!
​
# Usage
​
## Windows

Use [Zadig](http://zadig.akeo.ie/) to install the WinUSB driver on your bluetooth device.

Run wiimote_btstack.exe and wait for startup, then press Enter to start bluetooth discovery and hold down 1 + 2 on the remote to start pairing. Once a new remote has been paired it can be connected again without pairing by pressing any button on the remote.