# Zeitprisma – Sammelbefehle (Python-venv mit platformio, manifold3d, numpy, matplotlib, pyserial aktiv)
ENV   ?= muenze8
REF   := ref/StickS3.stl
REF_URL := https://raw.githubusercontent.com/m5stack/M5_Hardware/master/Products/K150_StickS3/Structures/StickS3.stl

.PHONY: test build flash log ref gehaeuse alle-gehaeuse clean

test:                       ## Host-Tests der Kernlogik (alle Bauformen)
	$(MAKE) -C firmware/test/host
	$(MAKE) -C firmware/test/host clean

build:                      ## Firmware fuer alle Bauformen bauen
	cd firmware && pio run -e muenze8 -e muenze12 -e prisma10

flash:                      ## Flashen, z. B. make flash ENV=muenze12
	cd firmware && pio run -e $(ENV) -t upload

log:                        ## 30 s serielle Ausgabe mitschreiben
	python3 tools/seriallog.py -s 30

$(REF):
	mkdir -p ref
	curl -fsSL -o $(REF) $(REF_URL)

ref: $(REF)                 ## offizielles StickS3-Strukturmodell laden (fuer --check)

gehaeuse: $(REF)            ## Standard-Muenze (8 Seiten, 22 mm) erzeugen + pruefen
	cd gehaeuse && python3 muenze_gehaeuse.py --check ../$(REF)

alle-gehaeuse: $(REF)       ## alle Varianten erzeugen + pruefen
	cd gehaeuse && python3 muenze_gehaeuse.py --check ../$(REF)
	cd gehaeuse && python3 muenze_gehaeuse.py --seiten 12 --dicke 18.58 --usb nein --check ../$(REF)
	cd gehaeuse && python3 prisma_gehaeuse.py --check ../$(REF)

clean:
	$(MAKE) -C firmware/test/host clean
	rm -rf firmware/.pio gehaeuse/__pycache__
