##librtsp
This is a simple librtsp library.

one file/stream: one media_source
one connect : one connect_session

file \ read                  write
     |=======> media_source =======> transport_session ||
live /                                                 || <= rtsp client request
                                                       ||
                                                       ||
                                                       ||
                                                       ||
