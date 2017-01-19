using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.IO;
using System.Linq;
using System.Net;
using System.Net.Sockets;
using System.Text;
using System.Threading.Tasks;

namespace LuaClient
{
    public enum LuaType
    {
        LUA_TNIL = 0,
        LUA_TBOOLEAN = 1,
        LUA_TLIGHTUSERDATA = 2,
        LUA_TNUMBER = 3,
        LUA_TSTRING = 4,
        LUA_TTABLE = 5,
        LUA_TFUNCTION = 6,
        LUA_TUSERDATA = 7,
        LUA_TTHREAD = 8
    }

    public class LuaValue
    {
        private object data;
        private LuaType type;
        public object Data { get { return data; } }
        public LuaType Type { get { return type; } }
        public LuaValue(double number)
        {
            data = number;
            type = LuaType.LUA_TNUMBER;
        }

        public LuaValue(string data)
        {
            this.data = data;
            type = LuaType.LUA_TSTRING;
        }

        public LuaValue(bool boolean)
        {
            data = boolean;
            type = LuaType.LUA_TBOOLEAN;
        }

        public override string ToString()
        {
            switch (type)
            {
                case LuaType.LUA_TSTRING:
                    return data as string;
                case LuaType.LUA_TBOOLEAN:
                    return ((bool)data).ToString();
                case LuaType.LUA_TNUMBER:
                    return ((double)data).ToString();
                default:
                    return base.ToString();
            }
        }
    }
    public class Client
    {
        protected class FrameStream
        {
            protected byte[] _data;
            public FrameStream(int size)
            {
                _data = new byte[size];
            }

            public static byte[] Piece(byte[] source, int start, int length)
            {
                byte[] part = new byte[length];
                for (int n = 0; n < length; n++)
                {
                    part[n] = source[n + start];
                }

                return part;
            }

            public void WriteAt(byte[] data, int offset, int length)
            {
                for (int n = 0; n < Math.Min(length, _data.Length); n++)
                {
                    _data[offset + n] = data[n];
                }
            }

            public void WriteAt(byte[] data, int offset, int length, int padcap)
            {
                for (int n = 0; n < padcap; n++)
                {
                    if (n >= length)
                    {
                        _data[offset + n] = 0;
                    }
                    else
                        _data[offset + n] = data[n];
                }
            }

            public byte[] GetBuffer()
            {
                return _data;
            }
        }

        private int headersize = (sizeof(uint) * 2) + 16;
        private int keysize = (sizeof(uint) * 4) + sizeof(int);

        public Exception LastError { get { return _ex; } }

        private Exception _ex;
        private string _addr;
        private int _port;
        private TcpClient _client;
        public Client(string address, int port)
        {
            _addr = address;
            _port = port;
            _client = null;
            Reconnect();
        }
        public bool Reconnect()
        {
            Disconnect();
            IPAddress[] ips = System.Net.Dns.GetHostAddresses(_addr);
            _client = new TcpClient();

            foreach (IPAddress ip in ips)
            {
                try
                {
                    _client.Connect(ip, _port);
                    return true;
                }
                catch (Exception ex) { _ex = ex; }
            }

            return false;
        }

        public bool Send(string method, Dictionary<string, LuaValue> data, int timeout)
        {
            int numbkeys = 0;
            int datasize = 0;
            int totalsize = 0;

            foreach (KeyValuePair<string, LuaValue> kv in data)
            {
                switch (kv.Value.Type)
                {
                    case LuaType.LUA_TNUMBER:
                        datasize += sizeof(double);
                        break;
                    case LuaType.LUA_TSTRING:
                        datasize += Encoding.Default.GetByteCount(kv.Value.Data as string);
                        break;
                    case LuaType.LUA_TBOOLEAN:
                        datasize += 1;
                        break;
                    default:
                        continue;
                }

                datasize += Encoding.Default.GetByteCount(kv.Key);
                numbkeys++;
            }

            totalsize = headersize + (keysize * numbkeys) + datasize;
            FrameStream frame = new FrameStream(totalsize);
            byte[] temp = BitConverter.GetBytes(totalsize);
            byte[] key;
            byte[] framedata;

            frame.WriteAt(temp, 0, sizeof(int));
            temp = Encoding.Default.GetBytes(method);
            frame.WriteAt(temp, sizeof(int), temp.Length, 16);

            temp = BitConverter.GetBytes(numbkeys);
            frame.WriteAt(temp, sizeof(int) + 16, sizeof(uint));

            int keyoffset = headersize;
            int dataoffset = keyoffset + (keysize * numbkeys);
            int fromzerodataoffset = 0;

            foreach (KeyValuePair<string, LuaValue> kv in data)
            {
                switch (kv.Value.Type)
                {
                    case LuaType.LUA_TNUMBER:
                        temp = BitConverter.GetBytes((double)kv.Value.Data);
                        break;
                    case LuaType.LUA_TSTRING:
                        temp = Encoding.Default.GetBytes(kv.Value.Data as string);
                        break;
                    case LuaType.LUA_TBOOLEAN:
                        temp = new byte[1];
                        if ((bool)kv.Value.Data)
                            temp[0] = 1;
                        else
                            temp[0] = 0;
                        break;
                    default:
                        continue;
                }

                key = Encoding.Default.GetBytes(kv.Key);

                framedata = BitConverter.GetBytes((int)kv.Value.Type);
                frame.WriteAt(framedata, keyoffset, sizeof(int));
                keyoffset += sizeof(int);

                framedata = BitConverter.GetBytes((int)fromzerodataoffset);
                frame.WriteAt(framedata, keyoffset, sizeof(uint));
                keyoffset += sizeof(uint);

                framedata = BitConverter.GetBytes(key.Length);
                frame.WriteAt(framedata, keyoffset, sizeof(uint));
                keyoffset += sizeof(uint);

                frame.WriteAt(key, dataoffset, key.Length);
                dataoffset += key.Length;
                fromzerodataoffset += key.Length;

                framedata = BitConverter.GetBytes((int)fromzerodataoffset);
                frame.WriteAt(framedata, keyoffset, sizeof(uint));
                keyoffset += sizeof(uint);

                framedata = BitConverter.GetBytes(temp.Length);
                frame.WriteAt(framedata, keyoffset, sizeof(uint));
                keyoffset += sizeof(uint);

                frame.WriteAt(temp, dataoffset, temp.Length);
                dataoffset += temp.Length;
                fromzerodataoffset += temp.Length;
            }

            int sent = 0;
            byte[] buffer = frame.GetBuffer();

            try
            {
                Stopwatch st = Stopwatch.StartNew();
                _client.Client.SendTimeout = timeout;
                while (sent < buffer.Length)
                {
                    if (st.ElapsedMilliseconds > timeout)
                        throw new TimeoutException();
                    sent += _client.Client.Send(buffer, sent, buffer.Length - sent, SocketFlags.None);
                }
            }
            catch (Exception ex)
            {
                _ex = ex;
                Disconnect();
                return false;
            }

            return true;
        }

        public Dictionary<string, LuaValue> Recv(out string method, int timeout)
        {
            Dictionary<string, LuaValue> data = new Dictionary<string, LuaValue>();
            method = null;
            byte[] packet = new byte[1500];
            int read = 0;

            try
            {
                if (_client.Client.Poll(timeout, SelectMode.SelectRead))
                {
                    read = _client.Client.Receive(packet, 24, SocketFlags.None);
                }
                else
                {
                    return null;
                }
            }
            catch (Exception ex)
            {
                _ex = ex;
                Disconnect();
                return null;
            }

            if (read <= 0 || read < 24)
            {
                return null;
            }

            uint length = BitConverter.ToUInt32(packet, 0);
            byte[] rawmethod = FrameStream.Piece(packet, sizeof(uint), 16);
            uint numbkeys = BitConverter.ToUInt32(packet, sizeof(uint) + 16);

            method = Encoding.Default.GetString(rawmethod);
            method = method.TrimEnd('\0');

            FrameStream stream = new FrameStream((int)length);
            stream.WriteAt(packet, 0, read);

            int total = read;
            try
            {
                _client.Client.ReceiveTimeout = timeout;
                Stopwatch st = Stopwatch.StartNew();
                while (total < length)
                {
                    if (st.ElapsedMilliseconds > timeout)
                        throw new TimeoutException();

                    if (_client.Client.Poll(timeout, SelectMode.SelectRead))
                    {                
                        read = _client.Client.Receive(packet, (int)length - total, SocketFlags.None);
                    }
                    else
                    {
                        return null;
                    }

                    stream.WriteAt(packet, total, read);
                    total += read;
                }
            }
            catch (Exception ex)
            {
                _ex = ex;
                Disconnect();
                return null;
            }

            int keyoffset = headersize;
            int dataoffset = keyoffset + ((int)numbkeys * keysize);

            int luatype;
            uint nameoffset;
            uint namelength;
            uint realdataoffset;
            uint datalength;
            string key;
            byte[] buffer = stream.GetBuffer();
            LuaValue val;
            LuaValue temp;
            for (int n = 0; n < (int)numbkeys; n++)
            {
                luatype = BitConverter.ToInt32(buffer, keyoffset);
                keyoffset += sizeof(int);
                nameoffset = BitConverter.ToUInt32(buffer, keyoffset);
                keyoffset += sizeof(uint);
                namelength = BitConverter.ToUInt32(buffer, keyoffset);
                keyoffset += sizeof(uint);
                realdataoffset = BitConverter.ToUInt32(buffer, keyoffset);
                keyoffset += sizeof(uint);
                datalength = BitConverter.ToUInt32(buffer, keyoffset);
                keyoffset += sizeof(uint);

                switch ((LuaType)luatype)
                {
                    case LuaType.LUA_TBOOLEAN:
                        val = new LuaValue(buffer[dataoffset + (int)realdataoffset] > 0);
                        break;
                    case LuaType.LUA_TNUMBER:
                        val = new LuaValue(BitConverter.ToDouble(buffer, dataoffset + (int)realdataoffset));
                        break;
                    case LuaType.LUA_TSTRING:
                        val = new LuaValue(Encoding.Default.GetString(buffer, dataoffset + (int)realdataoffset, (int)datalength));
                        break;
                    default:
                        continue;
                }

                key = Encoding.Default.GetString(buffer, dataoffset + (int)nameoffset, (int)namelength);
                if (data.TryGetValue(key, out temp))
                    data.Remove(key);
                data.Add(key, val);
            }

            return data;
        }

        public void Disconnect()
        {
            if (_client == null)
            {
                return;
            }

            try
            {
                _client.Close();
            }
            catch { }
            _client = null;
        }
    }
}
