//
//  main.c
//  chreso
//
//  Created by かおる on 2012/12/10.
//  Copyright (c) 2012年 かおる. All rights reserved.
//

#include <CoreFoundation/CoreFoundation.h>
#include <CoreGraphics/CoreGraphics.h>

static FILE* s_stdout = NULL;
static FILE* s_stderr = NULL;

// ---------------------------------------------------------
//  [Usage]
//    in  @ FILE* out
//    out @ none
static void Usage( FILE* fp ){
    fprintf(fp,"[Usage] chreso    -  showing main display modes.\n");
    fprintf(fp,"        chreso N  -  N is index of display mode to setup.\n");
}

// ---------------------------------------------------------
//  [CreateAllDisplayModeArray]
//    in  @ CGDirectDisplayID displayID
//    out @ CFArrayRef
static CFArrayRef CreateAllDisplayModeArray( CGDirectDisplayID displayID ){

    CFArrayRef arrayModes = NULL;
    
    do{
        int value = 1;
        
        CFNumberRef number = CFNumberCreate( kCFAllocatorDefault, kCFNumberIntType, &value );
        if( !number )   break;
        
        CFStringRef key = kCGDisplayShowDuplicateLowResolutionModes;
        
        CFDictionaryRef options = CFDictionaryCreate( kCFAllocatorDefault, (const void **)&key, (const void **)&number, 1, NULL, NULL );
        
        CFRelease(number);
        if( !options )  break;
        
        arrayModes = CGDisplayCopyAllDisplayModes( displayID, options );
        
        CFRelease(options);

    }while(false);
    
    return arrayModes;
}


// ---------------------------------------------------------
//  [ShowModeInfo]
//    in  @ int idx
//        @ CGDisplayModeRef mode
//        @ CGDisplayModeRef currentMode
//    out @ none
static void ShowModeInfo( int idx, CGDisplayModeRef mode, CGDisplayModeRef currentMode ){
    
    size_t width = CGDisplayModeGetWidth(mode);
    size_t height = CGDisplayModeGetHeight(mode);
    
    size_t pwidth = CGDisplayModeGetPixelWidth(mode);
    //size_t pheight = CGDisplayModeGetPixelHeight(mode);
    
    int depth = 0;
    
    CFStringRef pixelEncodeing = CGDisplayModeCopyPixelEncoding(mode);
    if( pixelEncodeing ){
        if(CFStringCompare(pixelEncodeing, CFSTR(IO32BitDirectPixels), kCFCompareCaseInsensitive) == kCFCompareEqualTo)
            depth = 32;
        else if(CFStringCompare(pixelEncodeing, CFSTR(IO16BitDirectPixels), kCFCompareCaseInsensitive) == kCFCompareEqualTo)
            depth = 16;
        else if(CFStringCompare(pixelEncodeing, CFSTR(IO8BitIndexedPixels), kCFCompareCaseInsensitive) == kCFCompareEqualTo)
            depth = 8;
        CFRelease(pixelEncodeing);
    }
    
    if( mode == currentMode ){
        fprintf( s_stdout, "*%3d: %4ld x %4ld - %2dbit - @%.0fx\n", idx, width, height, depth, ((double)pwidth) / ((double)width) );
    }else{
        fprintf( s_stdout, " %3d: %4ld x %4ld - %2dbit - @%.0fx\n", idx, width, height, depth, ((double)pwidth) / ((double)width) );
    }
}

// ---------------------------------------------------------
//  [ShowMode]
//    in  @ CGDirectDisplayID displayID
//    out @ int
static int ShowMode( CGDirectDisplayID displayID ){
    
    
    CFArrayRef arrayModes = CreateAllDisplayModeArray(displayID);
    if( !arrayModes ){
        fprintf( s_stderr, "ERROR! make display mode list.\n" );
        return EXIT_FAILURE;
    }
    
    CGDisplayModeRef currentMode = CGDisplayCopyDisplayMode(displayID);

    int count = (int)CFArrayGetCount(arrayModes);
    
    fprintf( s_stdout, "[ Display Modes ] - count:%d\n\n",count);
    
    for( int i = 0; i < count; i++ ){
        CGDisplayModeRef mode = (CGDisplayModeRef)CFArrayGetValueAtIndex( arrayModes, i );
        ShowModeInfo( i, mode, currentMode );
    }

    CFRelease(arrayModes);
    
    if( currentMode )   CGDisplayModeRelease(currentMode);

    fprintf( s_stdout, "\n" );
    Usage(s_stdout);
    
    return EXIT_SUCCESS;
}

// ---------------------------------------------------------
//  [Chreso]
//    in  @ const char* arg
//        @ CGDirectDisplayID displayID
//    out @ int
int Chreso( const char* arg, CGDirectDisplayID displayID ){

    char* endptr = NULL;
    
    long idx = strtol( arg, &endptr, 10 );
    
    if( *endptr != '\0' ){
        fprintf( s_stderr, "ERROR! invalid argument. \"%s\"\n", arg );
        return EXIT_FAILURE;
    }
    
    if( errno == ERANGE ){
        if( idx == LONG_MIN ){
            fprintf( s_stderr, "ERROR! argument value is less than value that can be represented. \"%s\"\n", arg );
            return EXIT_FAILURE;
        }else if( idx == LONG_MAX ){
            fprintf( s_stderr, "ERROR! argument value is greater than value that can be represented. \"%s\"\n", arg );
            return EXIT_FAILURE;
        }
    }
    
    if( idx < 0 ){
        fprintf( s_stderr, "ERROR! index is less than 0.\n" );
    }
    
    CFArrayRef arrayModes = CreateAllDisplayModeArray(displayID);
    if( !arrayModes ){
        fprintf( s_stderr, "ERROR! make display mode list.\n" );
        return EXIT_FAILURE;
    }

    int ret = EXIT_FAILURE;
    do{
    
        long count = CFArrayGetCount(arrayModes);
        
        if( idx >= count ){
            fprintf( s_stderr, "ERROR! index is greater than display mode list length.\n" );
            break;
        }

        CGDisplayModeRef mode = (CGDisplayModeRef)CFArrayGetValueAtIndex( arrayModes, idx );
        
        CGError error;
        CGDisplayFadeReservationToken token;
        
        //  fade out.
        error = CGAcquireDisplayFadeReservation( kCGMaxDisplayReservationInterval, &token );
        if( error != noErr ){
            fprintf( s_stderr, "ERROR! acquire display fade reservation. %d\n", error );
            break;
        }
        
        error = CGDisplayFade( token, 0.3, kCGDisplayBlendNormal, kCGDisplayBlendSolidColor, 0.0, 0.0, 0.0, true );
        if( error != noErr ){
            fprintf( s_stderr, "ERROR! display fade out. %d\n", error );
            break;
        }

        //  configure display.
        CGDisplayConfigRef config = NULL;
        
        do{
            error = CGBeginDisplayConfiguration( &config );
            if( error != noErr ){
                fprintf( s_stderr, "ERROR! begin display configuration. %d\n", error );
                break;
            }

            error = CGConfigureDisplayWithDisplayMode( config, displayID, mode, NULL );
            if( error != noErr ){
                fprintf( s_stderr, "ERROR! configure display mode. %d\n", error );
                break;
            }

            error = CGCompleteDisplayConfiguration( config, kCGConfigurePermanently );
            if( error != noErr ){
                fprintf( s_stderr, "ERROR! complete display configuration. %d\n", error );
                break;
            }
            
            config = NULL;
        }while(false);
        
        if( config )    CGCancelDisplayConfiguration(config);

        //  fade in.
        error = CGDisplayFade( token, 0.3, kCGDisplayBlendSolidColor, kCGDisplayBlendNormal, 0.0, 0.0, 0.0, false );
        if( error != noErr ){
            fprintf( s_stderr, "ERROR! display fade in. %d\n", error );
            break;
        }

        error = CGReleaseDisplayFadeReservation(token);
        if( error != noErr ){
            fprintf( s_stderr, "ERROR! release display fade reservation. %d\n", error );
            break;
        }
        
        fprintf( s_stdout, "Success change display resolution.\n" );
        ShowModeInfo( (int)idx, mode, NULL );
        ret = EXIT_SUCCESS;
    }while(false);
    
    return ret;
}

// ---------------------------------------------------------
//  [main]
//    in  @ int argc
//        @ const char* argv[]
//    out @ int
int main( int argc, const char* argv[] ){
    
    s_stdout = stdout;
    s_stderr = stderr;

    int ret = EXIT_FAILURE;
    
    switch( argc ){
        case 1:
            ret = ShowMode( CGMainDisplayID() );
            break;
        case 2:
            ret = Chreso( argv[1], CGMainDisplayID() );
            break;
        default:
            Usage( s_stderr );
            break;
    }

    return ret;
}
