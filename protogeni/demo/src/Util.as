package
{
  import flash.events.ErrorEvent;
  import com.mattism.http.xmlrpc.MethodFault;

  class Util
  {
    public static function getResponse(name : String, xml : String) : String
    {
      return "\n-----------------------------------------\n"
        + "RESPONSE: " + name + "\n"
        + "-----------------------------------------\n\n"
        + xml;
    }

    public static function getSend(name : String, xml : String) : String
    {
      return "\n-----------------------------------------\n"
        + "SEND: " + name + "\n"
        + "-----------------------------------------\n\n"
        + xml;
    }

    public static function getFailure(opName : String,
                                      event : ErrorEvent,
                                      fault : MethodFault) : String
    {
      if (fault != null)
      {
        return "FAILURE fault: " + opName + ": " + fault.getFaultString()
          + "\n";
      }
      else
      {
        return "FAILURE event: " + opName + ": " + event.toString() + "\n";
      }
    }
  }
}
