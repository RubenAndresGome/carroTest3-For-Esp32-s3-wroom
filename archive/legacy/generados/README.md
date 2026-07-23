# Snapshots regenerables

Los archivos `*.ino` y `*.txt` de esta carpeta son copias monolíticas para
consulta y están ignorados por Git. Se regeneran con:

```powershell
python archive\legacy\generadores\generar_archivos_sopa.py
```

Nunca deben editarse ni usarse como fuente canónica.
