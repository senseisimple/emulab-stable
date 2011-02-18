package com.mattism.http.xmlrpc
{
  /* An HTTPS URL Loader which uses the external Forge TLS library */

  import com.hurlant.crypto.hash.MD5;
  import com.hurlant.crypto.symmetric.CBCMode;
  import com.hurlant.crypto.symmetric.TripleDESKey;
  
  import flash.events.Event;
  import flash.events.EventDispatcher;
  import flash.events.IOErrorEvent;
  import flash.events.SecurityErrorEvent;
  import flash.external.ExternalInterface;
  import flash.net.URLLoader;
  import flash.net.URLRequest;
  import flash.net.URLRequestHeader;
  import flash.utils.ByteArray;
  import flash.utils.Dictionary;
  
  import mx.utils.Base64Decoder;
  import mx.utils.Base64Encoder;

  public class JSLoader extends URLLoader
  {
    public static function setServerCertificate(newCert : String) : void
    {
        ExternalInterface.call("setServerCert", newCert);
    }

    public static function setClientInfo(password : String,
                                         pem : String) : void
    {
      var lines : Array = pem.split('\n');
      var key : String = "";
      var cert : String = "";
      var inKey : Boolean = false;
      var inCert : Boolean = false;
      for each (var line : String in lines)
      {
        if (line == "-----BEGIN RSA PRIVATE KEY-----" && key == "")
        {
          inKey = true;
        }
        else if (line == "-----BEGIN CERTIFICATE-----" && cert == "")
        {
          inCert = true;
        }

        if (inKey)
        {
          key += line + "\n";
        }
        if (inCert)
        {
          cert += line + "\n";
        }

        if (line == "-----END RSA PRIVATE KEY-----")
        {
          inKey = false;
        }
        else if (line == "-----END CERTIFICATE-----")
        {
          inCert = false;
        }
      }
      var iv : ByteArray = generateIv(key);
      if (iv != null)
      {
        var desKey : ByteArray = generateDesKey(password, iv);
        var clearKey : String = decodeKey(desKey, iv, key);
        ExternalInterface.call("setClientCert", cert);
        ExternalInterface.call("setClientKey", clearKey);
      }
      else
      {
        Main.log.appendMessage(new LogMessage("error", "Invalid Key", key, true));
      }
    }

    private static function generateIv(key : String) : ByteArray
    {
      var result : ByteArray = null;
      var patterns : Array = null;
      var lines : Array = key.split("\n");
      for each (var line : String in lines)
      {
        patterns = line.match(/^DEK-Info: ([\-a-zA-Z0-9]+),([A-Za-z0-9]+)$/);
        if (patterns != null)
        {
          break;
        }
      }
      if (patterns != null)
      {
        var method : String = patterns[1];
        var iv : String = patterns[2];
        if (method == "DES-EDE3-CBC")
        {
          result = parseIv(iv);
        }
      }
      return result;
    }

    private static function parseIv(iv : String) : ByteArray
    {
      var result : ByteArray = new ByteArray();
      for (var i : int = 0; i < iv.length - 1; i += 2)
      {
        var byteStr : String = iv.slice(i, i + 2);
        var byte : int = parseInt("0x" + byteStr);
        result.writeByte(byte);
      }
      return result;
    }

    private static function generateDesKey(password : String,
                                           iv : ByteArray) : ByteArray
    {
      var key : ByteArray = new ByteArray();
      var keySize : int = 24;
      var salt : ByteArray = new ByteArray();
      salt.writeBytes(iv, 0, 8);

      var hashIn : ByteArray = new ByteArray();
      var md5 : MD5 = new MD5();
      hashIn.writeUTFBytes(password);
      hashIn.writeBytes(salt, 0, 8);
      var digest : ByteArray = md5.hash(hashIn);
      key.writeBytes(digest, 0, 16);

      hashIn = new ByteArray();
      hashIn.writeBytes(digest, 0, 16);
      hashIn.writeUTFBytes(password);
      hashIn.writeBytes(salt, 0, 8);
      digest = md5.hash(hashIn);
      key.writeBytes(digest, 0, 8);

      return key;
    }

    private static function decodeKey(desKey : ByteArray, iv : ByteArray,
                                      key : String) : String
    {
      var keyBinary : ByteArray = parseKey(key);
      var des : TripleDESKey = new TripleDESKey(desKey);
      var cbc : CBCMode = new CBCMode(des);
      cbc.IV = iv;
      cbc.decrypt(keyBinary);
//      cbc.dispose();
      return plainToString(keyBinary);
    }

    private static function parseKey(key : String) : ByteArray
    {
      var decoder : Base64Decoder = new Base64Decoder();
      var lines : Array = key.split("\n");
      for each (var line : String in lines)
      {
        if (! line.match("----") && ! line.match(": ") && line != "")
        {
          decoder.decode(line);
        }
      }
      return decoder.toByteArray();
    }

    private static function plainToString(plain : ByteArray) : String
    {
      var result : String = "-----BEGIN RSA KEY-----\n";
      var encoder : Base64Encoder = new Base64Encoder();
      encoder.encodeBytes(plain, 0, plain.length);
      result += encoder.toString();
      result += "-----END RSA KEY-----";
      return result;
    }

    public static function setClientCertificate(newCert : String) : void
    {
      ExternalInterface.call("setClientCert", newCert);
    }

    public static function setClientKey(newKey : String) : void
    {
      ExternalInterface.call("setClientKey", newKey);
    }

    public function JSLoader() : void
    {
      init(this);
      bytesLoaded = 0;
      data = "";
      id = NO_ID;
    }

    override public function load(request : URLRequest) : void
    {
      var sendData : String = request.data.toXMLString();
      var splitExp : RegExp = /^(https:\/\/[^\/]+)(\/.*)$/;
      var result : Array = splitExp.exec(request.url);
      var host : String = result[1];
      var path : String= result[2];
      if (id != NO_ID)
      {
        close();
      }
      id = getInstance(this);
      loadInstance(id, host, path, sendData);
    }

	override public function close() : void
    {
      cleanupInstance(id);
      id = NO_ID;
    }

    private function onBody(body : String) : void
    {
      data = body;
      bytesLoaded = body.length;
      close();
      dispatchEvent(new Event(Event.COMPLETE, false, false));
    }

    private function onError(message : String) : void
    {
      close();
      dispatchEvent(new IOErrorEvent(IOErrorEvent.IO_ERROR, false, false,
                                     message));
    }

	//override public var data : String;
	//override public var bytesLoaded : uint;

    private var id : int;

    private static var NO_ID : int = -1;

    static private var initialized : Boolean = false;
    static private var lastInstance : int = 0;
    static private var loaders : Dictionary;

    private static function init(obj : JSLoader) : void
    {
      if (! initialized)
      {
        initialized = true;
        loaders = new Dictionary();
        ExternalInterface.addCallback("flash_onbody", body_handler);
        ExternalInterface.addCallback("flash_onerror", error_handler);
        ExternalInterface.call("init", ExternalInterface.objectID);
      }
    }

    private static function getInstance(current : JSLoader) : int
    {
      var result : int = lastInstance;
      ++lastInstance;
      if (lastInstance > 1000000)
      {
        lastInstance = 0;
      }
      loaders[result] = current;
      return result;
    }

    private static function cleanupInstance(instance : int) : void
    {
      var current : JSLoader = loaders[instance];
      if (instance != NO_ID && current != null)
      {
        delete loaders[instance];
        ExternalInterface.call("cancel_request", instance);
      }
    }

    private static function loadInstance(instance : int, host : String,
                                         path : String,
                                         sendData : String) : void
    {
      ExternalInterface.call("make_request", instance, host, path, sendData);
    }

    private static function body_handler(instance : int, body : String) : void
    {
      var current : JSLoader = loaders[instance];
      if (current != null)
      {
        current.onBody(body);
      }
      else
      {
        cleanupInstance(instance);
      }
    }

    private static function error_handler(instance : int,
                                          message : String) : void
    {
      var current : JSLoader = loaders[instance];
      if (current != null)
      {
        current.onError(message);
      }
      else
      {
        cleanupInstance(instance);
      }
    }
  }
}
