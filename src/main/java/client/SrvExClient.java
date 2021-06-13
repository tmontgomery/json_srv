package client;

import org.agrona.BitUtil;
import org.agrona.IoUtil;
import org.agrona.SystemUtil;
import org.agrona.collections.MutableBoolean;
import org.agrona.concurrent.IdleStrategy;
import org.agrona.concurrent.UnsafeBuffer;
import org.agrona.concurrent.YieldingIdleStrategy;
import org.agrona.concurrent.ringbuffer.OneToOneRingBuffer;

import java.io.File;
import java.nio.MappedByteBuffer;

public class SrvExClient
{
    private static final int VERSION_OFFSET = 0;
    private static final int SRV_INBOUND_LENGTH_OFFSET = BitUtil.SIZE_OF_LONG;
    private static final int SRV_OUTBOUND_LENGTH_OFFSET = BitUtil.SIZE_OF_LONG + BitUtil.SIZE_OF_LONG;

    public static void main(final String[] args)
    {
        final File baseDir = SystemUtil.isLinux() ?
            new File("/dev/shm") :
            new File(SystemUtil.tmpDirName());
        final MappedByteBuffer srvFile = IoUtil.mapExistingFile(
            new File(baseDir, "srv.dat"), "service dat file");
        final UnsafeBuffer header = new UnsafeBuffer(srvFile, 0, BitUtil.CACHE_LINE_LENGTH);

        // TODO: depending on how started up, may need to loop on map and header until ready.

        final long srvInboundLength = header.getLong(SRV_INBOUND_LENGTH_OFFSET);
        final long srvOutboundLength = header.getLong(SRV_OUTBOUND_LENGTH_OFFSET);

        final UnsafeBuffer srvInboundRbBuffer = new UnsafeBuffer(
            srvFile, BitUtil.CACHE_LINE_LENGTH, (int)srvInboundLength);
        final UnsafeBuffer srvOutboundRbBuffer = new UnsafeBuffer(
            srvFile, BitUtil.CACHE_LINE_LENGTH + (int)srvInboundLength, (int)srvOutboundLength);

        final OneToOneRingBuffer srvInboundRb = new OneToOneRingBuffer(srvInboundRbBuffer);
        final OneToOneRingBuffer srvOutboundRb = new OneToOneRingBuffer(srvOutboundRbBuffer);
        final IdleStrategy idleStrategy = new YieldingIdleStrategy();

        final UnsafeBuffer srcBuffer = new UnsafeBuffer(new byte[1024]);
        final int messageLength = srcBuffer.putStringWithoutLengthAscii(0, "message the first");

        idleStrategy.reset();
        while (!srvInboundRb.write(1, srcBuffer, 0, messageLength))
        {
            idleStrategy.idle();
        }

        System.out.println("Message sent!");

        final MutableBoolean received = new MutableBoolean(false);
        while (!received.get())
        {
            srvOutboundRb.read((msgTypeId, buffer, index, length) ->
            {
                System.out.println("Message is: " + buffer.getStringWithoutLengthAscii(index, length));
                received.set(true);
            });
        }
    }
}
