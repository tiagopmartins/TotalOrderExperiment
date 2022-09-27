# Total Order Experiment

## Commands

### begin

Starts a message exchange between all servers during the amount of time specified.

```
begin seconds
```

### dump

Dumps the contents of the Redis database into a file. Supported types: [probing, logs]

```
dump type
```

### exit

Exits the program.

```
exit
```

### fetch

Fetches the time values obtained during the message exchange.

```
fetch
```

### get-servers

Fetches the list of active servers.

```
get-servers
```

### probe

Starts probing the specified address. Separates the value in time slots of one second.

```
probe address seconds
```