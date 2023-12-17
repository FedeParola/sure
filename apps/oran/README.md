```bash
ASN1C_PREFIX=LTE_ /opt/asn1c/bin/asn1c -pdu=all -fcompound-names -gen-UPER -no-gen-BER -no-gen-JER -no-gen-OER -gen-APER -no-gen-example -D ./generated ./ASN.1/lte-rrc-16.13.0.asn1

ASN1C_PREFIX=NR_ /opt/asn1c/bin/asn1c -pdu=all -fcompound-names -gen-UPER -no-gen-BER -no-gen-JER -no-gen-OER -gen-APER -no-gen-example -findirect-choice -D ./generated ./ASN.1/nr-rrc-17.3.0.asn1

ASN1C_PREFIX=NGAP_ /opt/asn1c/bin/asn1c -pdu=all -fcompound-names -gen-APER -no-gen-BER -no-gen-JER -no-gen-OER -gen-UPER -no-gen-example -fno-include-deps -findirect-choice -D ./generated ./ASN1/ngap-15.8.0.asn1

ASN1C_PREFIX=S1AP_ /opt/asn1c/bin/asn1c -gen-APER -no-gen-BER -no-gen-JER -no-gen-OER -gen-UPER -fcompound-names -no-gen-example -fno-include-deps -D ./generated ./ASN1/R15/s1ap-15.6.0.asn1

ASN1C_PREFIX=E1AP_ /opt/asn1c/bin/asn1c -gen-APER -gen-UPER -no-gen-JER -no-gen-BER -no-gen-OER -fcompound-names -no-gen-example -findirect-choice -fno-include-deps -D ./generated ./ASN.1/38463-g80.R16.78.0.asn

ASN1C_PREFIX=F1AP_ /opt/asn1c/bin/asn1c -gen-APER -no-gen-BER -no-gen-JER -no-gen-OER -gen-UPER -fcompound-names -no-gen-example -findirect-choice -fno-include-deps -D ./generated ./ASN1/R16.3.1/38473-g31.asn

ASN1C_PREFIX=X2AP_ /opt/asn1c/bin/asn1c -pdu=all -gen-APER -no-gen-BER -no-gen-JER -no-gen-OER -gen-UPER -fcompound-names -no-gen-example -fno-include-deps -D ./generated ./ASN1/R15/x2ap-15.6.0.asn1

ASN1C_PREFIX=M2AP_ /opt/asn1c/bin/asn1c -gen-APER -no-gen-BER -no-gen-JER -no-gen-OER -gen-UPER -fcompound-names -no-gen-example -fno-include-deps -D ./generated ./ASN1/m2ap-14.0.0.asn

ASN1C_PREFIX=M3AP_ /opt/asn1c/bin/asn1c -gen-APER -no-gen-BER -no-gen-JER -no-gen-OER -gen-UPER -fcompound-names -no-gen-example -fno-include-deps -D ./generated ./ASN1/m3ap-14.0.0.asn
```