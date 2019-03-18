../storage-system/server/master &
sleep 0.1
../storage-system/server/slave 127.0.0.1:50051 127.0.0.1:10000 127.0.0.1:10000 &
sleep 0.1
../storage-system/server/slave 127.0.0.1:50051 127.0.0.1:20000 127.0.0.1:10000 &
sleep 0.1
../storage-system/server/slave 127.0.0.1:50051 127.0.0.1:30000 127.0.0.1:10000 &
sleep 0.1
../storage-system/server/slave 127.0.0.1:50051 127.0.0.1:10001 127.0.0.1:10001 &
sleep 0.1
../storage-system/server/slave 127.0.0.1:50051 127.0.0.1:20001 127.0.0.1:10001 &
sleep 0.1
../storage-system/server/slave 127.0.0.1:50051 127.0.0.1:10002 127.0.0.1:10002 &
sleep 0.1
../storage-system/server/slave 127.0.0.1:50051 127.0.0.1:20002 127.0.0.1:10002 &
sleep 0.1
./Servant 80 &
