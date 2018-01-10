# Debug things

# Always make sure you set something via LCFLAGS +=
LCFLAGS = --leak-check=full

# Always make sure you set something via DBFLAGS +=
DBFLAGS = -ex run --args

# Debug target
debug: nw 
debug:
	$(DB) $(DBFLAGS) $(RUNARGS)

# Leak check target
leak: nw 
leak:
	$(LC) $(LCFLAGS) $(RUNARGS)
