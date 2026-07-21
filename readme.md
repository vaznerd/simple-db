# sdb

A simple key-value database CLI written in C. Data is stored as flat `key=value` pairs in a local text file.

## Features

- Store and retrieve key-value pairs
- Set, get, update, and delete individual pairs
- Add multiple pairs in a single `set` command
- Deduplicate entries in the database

## Building

Prerequisites: a C compiler (clang or gcc), Make.

```sh
make        # build the sdb binary
make clean  # remove the binary
```

## Usage

```
sdb set key=value [key2=value2 ...]   Set one or more key-value pairs
sdb get key                           Get the value for a key
sdb del key                           Delete a key-value pair
sdb update key=newvalue               Update the value of an existing key
sdb dedupe                            Remove duplicate entries from the database
sdb -h, --help                        Show help
```

### Examples

```sh
sdb set name=piyush
sdb set color=blue lang=c
sdb get name
sdb update name=jane
sdb del name
sdb dedupe
```

## Data Storage

All data is stored at `~/.local/share/sdb/database.txt`, with one `key=value` pair per line. The directory is created automatically on first use.

## License

[MIT](https://opensource.org/licenses/MIT)
