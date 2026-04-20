# eBookReaderSwitch

### Features:
* Saves last page number
* Reads PDF, EPUB, CBZ, and XPS files
* Dark and light mode
* Landscape reading view
* Portrait reading view
* Touch screen controls
	* Touch the botton/top of the screen to zoom in/out and left and right to change the page.
* Books read from `/switch/eBookReader/books`

### TODO:
* Do some extra testing on file compatibility.
* 2 pages side by side in landscape.
* Hardware lock to prevent accidental touches (maybe Vol- ?) (?).
* Save orientation, and dark mode settings.

### Screen Shots:

Dark Mode Help Menu:
<br></br>
<img src="screenshots/darkModeHelp.jpg" width="322" height="178.4">
<br></br>

Dark Mode Landscape Reading (With the Switch horizonal):
<br></br>
<img src="screenshots/darkModeLandscape.jpg" width="512" height="288">
<br></br>

Dark Mode Portrait Reading (With the Switch vertical):
<br></br>
<img src="screenshots/darkModePortrait.jpg" width="285.6" height="408.8">
<br></br>

Dark Mode Book Selection:
<br></br>
<img src="screenshots/darkModeSelection.jpg" width="512" height="288">
<br></br>

Light Mode Landscape Reading:
<br></br>
<img src="screenshots/lightModeLandscape.jpg" width="512" height="288">

### Credit:
* moronigranja - For allowing more file support
* NX-Shell Team - A good amount of the code is from an old version of their application.

### Building (Debian/Ubuntu)

#### 1. Install devkitPro

```bash
sudo apt install build-essential make pkg-config

# Add devkitPro APT repository
sudo mkdir -p /usr/share/keyring/
sudo wget -U "dkp apt" -O /usr/share/keyring/devkitpro-pub.gpg https://apt.devkitpro.org/devkitpro-pub.gpg
echo "deb [signed-by=/usr/share/keyring/devkitpro-pub.gpg] https://apt.devkitpro.org stable main" | sudo tee /etc/apt/sources.list.d/devkitpro.list
sudo apt-get update

# Install Switch toolchain and portlibs
sudo apt-get install devkitpro-pacman
sudo dkp-pacman -S switch-dev libnx switch-portlibs
```

#### 2. Build

```bash
export DEVKITPRO=/opt/devkitpro

make
```

#### 3. Clean

```bash
make clean
```

The output `.nro` file will be in the project root.

If you don't have twili debugger installed, build with `NODEBUG=1`:
```bash
NODEBUG=1 make
```
