package 
{
import mx.core.MovieClipLoaderAsset;
import flash.utils.ByteArray;

public class GeneratedAsset9 extends MovieClipLoaderAsset
{
	public function GeneratedAsset9()
	{
		super();
		initialWidth=320/20;
		initialHeight=320/20;
	}
	private static var bytes:ByteArray = null;

	override public function get movieClipData():ByteArray
	{
		if (bytes == null)
		{
			bytes = ByteArray( new dataClass() );
		}
		return bytes;
	}

	[Embed(_resolvedSource='D:/FLUX/testbed/protogeni/demo/map/images/waiting.swf', mimeType='application/octet-stream')]
	public var dataClass:Class;
}
}
