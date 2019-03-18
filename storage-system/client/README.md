## How to Run
  1. follow the [grpc install instruction](https://github.com/grpc/grpc/blob/master/INSTALL.md)
  2. `cd ../server`
  3. `make`
  4. start the master server `./master` (starting the master node `127.0.0.1:50051`)
  5. start a slave `./slave 127.0.0.1:50051 127.0.0.1:10000 127.0.0.1:10000` ([usage]: ./slave [master address] [slave address] [replication address])
  6. `cd ../client`
  7. `make`
  8. `./sample`

  The output should be:

  ```
  Get a single greeting by row key --- greeting1 = Hello Bigtable!
  check_and_put returns true
  table[greeting1][column-family-1][column-1] has been updated to "new content"
  check_and_put returns false
  table[greeting0][column-family-1][column-1] is still "Hello World!"
  p->content = Hey!
  p->id = 5
  p->flag = false
  p->v = { v[0] v[1] v[2] }
  Delete the table
  ```

## Docs
  This is our storage client implementation, which is pretty similar to HBase API.

  Take a look at the [HBase sample program](https://cloud.google.com/bigtable/docs/samples-java-hello).

  sample.cc is an example program.
  sample3.cc is for stress testing.