typedef char buf <>;
struct pair {
	buf key;
	buf value;
};

 program KVSTORE {
	version KVSTORE_V1 {
		int EXAMPLE(int) = 1;
		string ECHO(string) = 2;
		void PUT(struct pair) = 3;
		buf GET(buf) = 4;
	} = 1;
} = 0x20000001;
