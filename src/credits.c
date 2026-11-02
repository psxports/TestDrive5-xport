#include <string.h>

#include <stdio.h>
#include <stdlib.h>

#include "credits.h"
#include "game.h"

CREDITS_STATE credits;

static const char text_0[] = "";
static const char text_1[] = "cPROGRAMMED BY";
static const char text_2[] = "aCHRIS KIRBY";
static const char text_3[] = "aHEADLEY LAMARR";
static const char text_4[] = "aBOB TROUGHTON";
static const char text_5[] = "aMIKE TROUGHTON";
static const char text_6[] = "cART BY";
static const char text_7[] = "aRICHARD BESTON";
static const char text_8[] = "aLES BURNEY";
static const char text_9[] = "aSTEVE DIETZ";
static const char text_10[] = "aJONATHAN F.KAY";
static const char text_11[] = "aRICHARD MCDONALD";
static const char text_12[] = "aMIKE PIRSO";
static const char text_13[] = "aTONY PRINGLE";
static const char text_14[] = "aJOHN STEELE";
static const char text_15[] = "aDAVID TAYLOR";
static const char text_16[] = "cTESTING AND AI MONKEYS";
static const char text_17[] = "aDAZ KELLY";
static const char text_18[] = "aTONY CHARLTON";
static const char text_19[] = "cPITBULL OFFICE MANAGER";
static const char text_20[] = "aDAVID BURTON";
static const char text_21[] = "cPRODUCER";
static const char text_22[] = "aSLADE ANDERSON";
static const char text_23[] = "cASSISTANT PRODUCER";
static const char text_24[] = "cEXTRA AI AND ALL ROUND";
static const char text_25[] = "cGOOD BLOKE";
static const char text_26[] = "aMATTHEW GUZENDA";
static const char text_27[] = "cLEAD ACCOLADE TESTER";
static const char text_28[] = "aMARIE PERSON";
static const char text_29[] = "cEXECUTIVE PRODUCER";
static const char text_30[] = "aCHRIS DOWNEND";
static const char text_31[] = "ecADDITIONAL PROGRAMMING";
static const char text_32[] = "GARETH BRIGGS";
static const char text_33[] = "STEVE PALMER";
static const char text_34[] = "cADDITIONAL TESTING";
static const char text_35[] = "RAYMOND HERRERA";
static const char text_36[] = "cSPECIAL THANKS TO";
static const char text_37[] = "CHARMAINE KIRBY";
static const char text_38[] = "DAWN 'TROUGHTON' TO BE";
static const char text_39[] = "SHIELAH PARR";
static const char text_40[] = "EMILY";
static const char text_41[] = "JESSILYN GUZENDA";
static const char text_42[] = "MICHELLE RAIN";
static const char text_43[] = "HAIRY PALMER SPELT 'TROUGHTON'";
static const char text_44[] = "PAULA";
static const char text_45[] = "DAVID 'GIS A CUDDLE' TAYLOR";
static const char text_46[] = "LISA MORROW";
static const char text_47[] = "JACKA AND RUTH";
static const char text_48[] = "NEIL DIETZ";
static const char text_49[] = "EMMA PALMER";
static const char text_50[] = "KYLE";
static const char text_51[] = "KAYLEIGH";
static const char text_52[] = "KERRY";
static const char text_53[] = "WOMPA";
static const char text_54[] = "KELLY O'HALORAN";
static const char text_55[] = "JOHN, JEANETTE AND ASLEY JASPER";
static const char text_56[] = "NEIL AND DOREEN";
static const char text_57[] = "SUE AND TOM";
static const char text_58[] = "TOMMY, BEN , TABITHA AND SAM ROBERTS";
static const char text_59[] = "AUXY";
static const char text_60[] = "DOLLY";
static const char text_61[] = "ROS MOONEY, JADE PIRSO, MICHELLE PIRSO";
static const char text_62[] = "COLIN 'SPORTS CAR' SMITH";
static const char text_63[] = "MIKE PIRSO'S MATES AND FAMILY";
static const char text_64[] = "PETER HAYNES";
static const char text_65[] = "RAYMOND OF FEAR FACTORY";
static const char text_66[] = "MONUMENT AQUATICS";
static const char text_67[] = "FEF AND JEE AND THE MELON JELLYS";
static const char text_68[] = "TONY CHARLTON THANKS";
static const char text_69[] = "DEB, BEZ, ANDEVERYONE WHO HAD FAITH IN HIM";
static const char text_70[] = "HELLO TO ALL FELLOW EWOKS";
static const char text_71[] = "CHRIS WILLIAMS";
static const char text_72[] = "HEADLEY LAMAR'S STUNT DOUBLE";
static const char text_73[] = "JAMES PARR";
static const char text_74[] = "RICK PARR 'NICE TD4 SITE'";
static const char text_75[] = "MARK, BECKY, RACHEAL, JAMES";
static const char text_76[] = "PAUL, SHERYL, PETER";
static const char text_77[] = "BEN SAMUELSON";
static const char text_78[] = "SPIKE";
static const char text_79[] = "CK'S MOGGYS";
static const char text_80[] = "TONY AT HEXHAM HORSELESS CARRIAGES";
static const char text_81[] = ".............";
static const char text_82[] = "PITBULL ALSO GIVES";
static const char text_83[] = "A SPECIAL THANKYOU";
static const char text_84[] = "TO JESSILYN AND";
static const char text_85[] = "MATTHEW GUZENDA";
static const char text_86[] = "cACCOLADE CREDITS";
static const char text_87[] = "cSENIOR PRODUCT MARKETING MANAGER";
static const char text_88[] = "STEVE ALLISON";
static const char text_89[] = "cINTERNATIONAL LIAISON";
static const char text_90[] = "JASON COHEN";
static const char text_91[] = "cUSA MASTERING SUPERVISOR";
static const char text_92[] = "LUIS RIVAS";
static const char text_93[] = "cCONSULTANT";
static const char text_94[] = "JEFF TAWNEY";
static const char text_95[] = "cTEST MANAGER";
static const char text_96[] = "BRIAN GILMER";
static const char text_97[] = "cTESTERS";
static const char text_98[] = "JASON LEVAN, DONNY CLAY, STEFANO CANU";
static const char text_99[] = "JAMES STRAWN, ARIF SINAN, JASON CORDERO";
static const char text_100[] = "CHRISTOPHER REIMER";
static const char text_101[] = "cINTRO CINEMATIC PRODUCER";
static const char text_102[] = "cMUSICAL SOUNDTRACK COMPILED BY";
static const char text_103[] = "cLICENSING MANAGER";
static const char text_104[] = "GABRIELLE BENHAM";
static const char text_105[] = "cLICENSING AND MEDIA SPECIALIST";
static const char text_106[] = "CHRISTINE LUGTON";
static const char text_107[] = "cDOCUMENTATION";
static const char text_108[] = "W.D. ROBINSON";
static const char text_109[] = "cWEB SITE DEVELOPMENT";
static const char text_110[] = "RAY MASSA, DANIEL GROVE";
static const char text_111[] = "cMARKETING SERVICES";
static const char text_112[] = "MATT ABRAMS, MARK GLOVER,";
static const char text_113[] = "JILL DOS SANTOS";
static const char text_114[] = "SPECIAL THANKS";
static const char text_115[] = "JIM BARNETT, STAN ROACH, NEIL JOHNSTON,";
static const char text_116[] = "ERICA KRISHNAMURTHY, JACK SYMON,";
static const char text_117[] = "JOHN KORONAIOS, STACY LAWRENCE";
static const char text_118[] = "JEAN RAYMOND, WAYNE LEONARD, JOHN LYMBERG,";
static const char text_119[] = "NH ROBINSON, GREG SARRAIL, LADDIE ERVIN,";
static const char text_120[] = "RICHARD KOTTLE, KATHY FAITH,";
static const char text_121[] = "JULIE DALTON, JOANNE MCKENSIE,";
static const char text_122[] = "STEVE SALEEN, GRAHAM NEARN,";
static const char text_123[] = "AVA ROTHENBERG, JOAN BOLVIN,";
static const char text_124[] = "JUDI KNIGHT, DON TERLOUSH,";
static const char text_125[] = "JIM PAWLIKE, JOHN ONEIL,";
static const char text_126[] = "LISA SHAMUS, BEN SAMUELSON,";
static const char text_127[] = "JOHN MARIES, STEVE TEMPLE,";
static const char text_128[] = "KRIS ANDERSON, SUMMER ANDERSON,";
static const char text_129[] = "LISA AND SIMON,";
static const char text_130[] = "LOUISE AT COUNTRY WIDE LETTING,";
static const char text_131[] = "VIPER CLUB OF AMERICA PACIFIC";
static const char text_132[] = "NORTHWEST REGION,";
static const char text_133[] = "SCOTT SNYDER, RUMP ROAST, BRETT WILKINSON";
static const char text_134[] = "LISTER, RIMMER, CAT, KRYTON AND BLK ADDER";
static const char text_135[] = "b";

static const char *const credits_lines[366] = {
    text_0, text_1,  text_0, text_0, text_0,  text_0,  text_0, text_2,  text_0,  text_0,  text_0,  text_0,  text_0, text_3,  text_0,  text_0,  text_0,  text_0,  text_0, text_4,  text_0,  text_0,  text_0,   text_0, text_0,   text_5,  text_0,  text_0,   text_0,  text_0, text_0,   text_0,   text_0,  text_0,   text_0,   text_0, text_6,   text_0,   text_0,  text_0,   text_0,   text_0,  text_7,   text_0,   text_0,   text_0,  text_0,   text_0,  text_8,   text_0,   text_0,   text_0,  text_0,   text_0,  text_9,   text_0,  text_0,   text_0,   text_0,   text_0,   text_10,  text_0,   text_0,   text_0,   text_0,  text_0,   text_11,  text_0,  text_0,   text_0,  text_0,   text_0,   text_12, text_0,   text_0,   text_0,  text_0,  text_0, text_13, text_0,  text_0,  text_0,  text_0,  text_0,  text_14, text_0,  text_0,  text_0,  text_0,   text_0,  text_15, text_0,
    text_0, text_0,  text_0, text_0, text_0,  text_0,  text_0, text_0,  text_0,  text_16, text_0,  text_0,  text_0, text_0,  text_0,  text_17, text_0,  text_0,  text_0, text_0,  text_0,  text_18, text_0,   text_0, text_0,   text_0,  text_0,  text_0,   text_0,  text_0, text_0,   text_0,   text_19, text_0,   text_0,   text_0, text_0,   text_0,   text_20, text_0,   text_0,   text_0,  text_0,   text_0,   text_0,   text_0,  text_0,   text_0,  text_0,   text_21,  text_0,   text_0,  text_0,   text_0,  text_0,   text_22, text_0,   text_0,   text_0,   text_0,   text_0,   text_0,   text_0,   text_0,   text_0,  text_0,   text_23,  text_24, text_25,  text_0,  text_0,   text_0,   text_0,  text_0,   text_26,  text_0,  text_0,  text_0, text_0,  text_0,  text_0,  text_0,  text_0,  text_0,  text_27, text_0,  text_0,  text_0,  text_0,   text_0,  text_28, text_0,
    text_0, text_0,  text_0, text_0, text_0,  text_0,  text_0, text_0,  text_29, text_0,  text_0,  text_0,  text_0, text_0,  text_30, text_0,  text_0,  text_0,  text_0, text_0,  text_0,  text_0,  text_0,   text_0, text_31,  text_0,  text_32, text_0,   text_33, text_0, text_0,   text_34,  text_0,  text_35,  text_0,   text_0, text_36,  text_37,  text_38, text_39,  text_40,  text_41, text_42,  text_43,  text_44,  text_45, text_46,  text_47, text_48,  text_49,  text_50,  text_51, text_52,  text_53, text_54,  text_55, text_56,  text_57,  text_58,  text_59,  text_60,  text_61,  text_62,  text_63,  text_64, text_65,  text_66,  text_67, text_0,   text_68, text_69,  text_70,  text_0,  text_71,  text_0,   text_72, text_73, text_0, text_74, text_75, text_76, text_77, text_78, text_79, text_80, text_81, text_82, text_83, text_84,  text_85, text_0,  text_0,
    text_0, text_86, text_0, text_0, text_87, text_88, text_0, text_89, text_90, text_0,  text_91, text_92, text_0, text_93, text_94, text_0,  text_95, text_96, text_0, text_97, text_98, text_99, text_100, text_0, text_101, text_88, text_0,  text_102, text_88, text_0, text_103, text_104, text_0,  text_105, text_106, text_0, text_107, text_108, text_0,  text_109, text_110, text_0,  text_111, text_112, text_113, text_0,  text_114, text_0,  text_115, text_116, text_117, text_0,  text_118, text_0,  text_119, text_0,  text_120, text_121, text_122, text_123, text_124, text_125, text_126, text_127, text_0,  text_128, text_129, text_0,  text_130, text_0,  text_131, text_132, text_0,  text_133, text_134, text_0,  text_0,  text_0, text_0,  text_0,  text_0,  text_0,  text_0,  text_0,  text_0,  text_0,  text_0,  text_0,  text_135, NULL,
};

void credits_prepare(void)
{
    uint8 *found_tex;
    uint8 *resource;
    uint8 *background;
    uint32 idx, size, src, entry_offset, val, unpacked, result, src_offset;

    FUNCTION_MARKER(0x800A73CC, "SLUS_006.10");
    credits.viewport_flags = (uint32)(768);
    for (idx = 0; idx < 11; ++idx)
    {
        credits.rows[idx].y = (uint16)(idx * 24);
        credits.rows[idx].text = credits_lines[0];
    }
    credits.cursor = (uint32)(0);
    credits.image_idx = (uint32)(0);
    audio_stop_xa();
    val = audio_options.music_volume;
    result = (uint32)audio_play_request(13, 20, (sint32)val, 0, 5120);
    size = archive_aligned_size("CRED\\CRED.DAT");
    XASSERT(!(credits.archive.bytes != NULL || size == 0));
    credits.archive.bytes = (uint8 *)malloc(size);
    XASSERT(!(credits.archive.bytes == NULL));
    credits.archive.capacity = size;
    credits.archive.size = archive_read_file("CRED\\CRED.DAT", credits.archive.bytes, size);
    resource = credits.archive.bytes;
    XASSERT(!(credits.archive.size < 8 || credits.archive.size > credits.archive.capacity));
    memcpy(&val, resource, sizeof(val));
    XASSERT(!(val == 0 || val > (credits.archive.size - 4) / 4));
    entry_offset = 4;
    idx = 1;
    do
    {
        XASSERT(!(entry_offset > credits.archive.size || credits.archive.size - entry_offset < 4));
        memcpy(&val, resource + entry_offset, sizeof(val));
        XASSERT(!(val > credits.archive.size || credits.archive.size - val < 18));
        memcpy(&val, resource, sizeof(val));
        ++idx;
        entry_offset += 4;
    } while (idx <= val);
    resource = credits.archive.bytes;
    memcpy(&src, resource + 4, sizeof(src));
    src_offset = src;
    unpacked = rnc_allocation_size(credits.archive.bytes + src_offset);
    resource = credits.archive.bytes;
    background = (uint8 *)presentation_background.image;
    memcpy(&src, resource + 4, sizeof(src));
    src_offset = src;
    XASSERT(!(src_offset > credits.archive.size || unpacked > sizeof(presentation_background.image) || unpacked < 1024 + 20 + 512 * 256 * 2));
    rnc_decode(credits.archive.bytes + src_offset, credits.archive.size - src_offset, background, (sint32)unpacked);
    found_tex = tex_find("Hicar");
    credits.tex = found_tex;
}

static sint32 read_image_dimensions(const uint8 *image, uint32 size, uint32 *width, uint32 *height)
{
    uint32 magic, pal_block_size, block_offset, block_size, bytes;
    sint16 pal_width, pal_height, image_width, image_height;

    FUNCTION_MARKER(0x80048F6C, "SLUS_006.10");
    if (size < 20)
        return 0;
    magic = (uint32)image[0] | ((uint32)image[1] << 8) | ((uint32)image[2] << 16) | ((uint32)image[3] << 24);
    if (magic != 16)
        return 0;
    pal_block_size = ((uint32)image[8] | ((uint32)image[9] << 8) | ((uint32)image[10] << 16) | ((uint32)image[11] << 24)) & 0xFFFFFFFC;
    if (pal_block_size < 12 || pal_block_size > size - 8)
        return 0;
    pal_width = ((uint16)image[16] | ((uint16)image[17] << 8));
    pal_height = ((uint16)image[18] | ((uint16)image[19] << 8));
    bytes = pal_width > 0 && pal_height > 0 ? (uint32)pal_width * (uint32)pal_height * 2 : 0;
    if (bytes > pal_block_size - 12)
        return 0;
    block_offset = 8 + pal_block_size;
    if (size - block_offset < 12)
        return 0;
    block_size = (uint32)image[block_offset] | ((uint32)image[block_offset + 1] << 8) | ((uint32)image[block_offset + 2] << 16) | ((uint32)image[block_offset + 3] << 24);
    if (block_size < 12 || block_size > size - block_offset)
        return 0;
    image_width = ((uint16)image[block_offset + 8] | ((uint16)image[block_offset + 9] << 8));
    image_height = ((uint16)image[block_offset + 10] | ((uint16)image[block_offset + 11] << 8));
    bytes = image_width > 0 && image_height > 0 ? (uint32)image_width * (uint32)image_height * 2 : 0;
    if (bytes > block_size - 12)
        return 0;
    *width = (uint32)(sint32)image_width << 2;
    *height = image_height;
    return 1;
}

static void resume_menu_music(void)
{
    FUNCTION_MARKER(0x80055A34, "SLUS_006.10");
    audio_play_xa(audio_xa.track);
}

static void upload_credit_image(uint32 idx)
{
    uint32 wire_word_6, wire_word_8;
    const uint8 *table;
    uint32 table_offset, src_offset, src, unpacked, page, clut;
    uint8 *image;
    uint8 *tex;
    uint32 image_x, image_y, pal_x, pal_y, val, width, height;

    FUNCTION_MARKER(0x800A754C, "SLUS_006.10");
    table = credits.archive.bytes;
    {
        sint32 operation_ok = !(credits.archive.size < 8 || idx >= (memcpy(&wire_word_6, table, sizeof(wire_word_6)), wire_word_6) - 1 || idx > (credits.archive.size - 8) / 4);
        XASSERT(operation_ok);
    }
    table_offset = idx * 4 + 8;
    XASSERT(!(table_offset > credits.archive.size || credits.archive.size - table_offset < 4));
    memcpy(&src, table + table_offset, sizeof(src));
    src_offset = src;
    XASSERT(!(src_offset > credits.archive.size || credits.archive.size - src_offset < 8));
    unpacked = rnc_allocation_size(credits.archive.bytes + src_offset);
    image = (uint8 *)malloc(unpacked);
    XASSERT(!(image == NULL));
    table = credits.archive.bytes;
    {
        sint32 operation_ok = !(credits.archive.size < 8 || idx >= (memcpy(&wire_word_8, table, sizeof(wire_word_8)), wire_word_8) - 1 || idx > (credits.archive.size - 8) / 4);
        XASSERT(operation_ok);
    }
    table_offset = idx * 4 + 8;
    XASSERT(!(table_offset > credits.archive.size || credits.archive.size - table_offset < 4));
    memcpy(&src, table + table_offset, sizeof(src));
    src_offset = src;
    XASSERT(!(src_offset > credits.archive.size));
    rnc_decode(credits.archive.bytes + src_offset, credits.archive.size - src_offset, image, (sint32)unpacked);
    // Exclude the allocation reserve from the TIM payload
    unpacked -= 1024;
    tex = credits.tex;
    page = ((const TEXTURE_DESCRIPTOR *)tex)->page;
    val = tex[14];
    image_x = ((page << 6) & 0x3FF) + (val >> 1);
    image_y = ((page << 4) & 0x100) + ((page >> 2) & 0x200);
    val = tex[15];
    clut = ((const TEXTURE_DESCRIPTOR *)tex)->clut;
    image_y |= val;
    pal_y = (clut >> 6) & 0xFF;
    pal_x = (clut & 0x3F) << 4;
    {
        sint32 operation_ok = !(!read_image_dimensions(image, unpacked, &width, &height));
        XASSERT(operation_ok);
    }
    tex_upload_tim(image, unpacked, (sint16)image_x, (sint16)image_y, (sint16)pal_x, (sint16)pal_y);
    free(image);
    val = width;
    tex = credits.tex;
    tex[12] = (((sint32)val / 2));
    tex = credits.tex;
    val = height;
    tex[13] = (val);
    credits.viewport_flags = (uint32)(256);
}

static void restore_credits_background(void)
{
    char filename[sizeof(presentation_background_format)];
    uint8 *buffer = credits.archive.bytes;
    uint32 capacity = credits.archive.capacity;
    uint32 val, x, y;
    sint32 remainder, len;

    FUNCTION_MARKER(0x800A78C0, "SLUS_006.10");
    if (buffer == NULL)
        return;
    credits.archive.bytes = NULL;
    credits.archive.size = 0;
    credits.archive.capacity = 0;
    val = presentation_background.cycle + 1;
    remainder = (sint32)val % 6;
    presentation_background.cycle = (uint32)((uint32)remainder);
    len = sprintf(filename, presentation_background_format, (sint32)((uint32)remainder + 1));
    {
        sint32 operation_ok = !(archive_read_file(filename, presentation_background.image, (uint32)sizeof(presentation_background.image)) < 20 + 512 * 256 * 2);
        XASSERT(operation_ok);
    }
    presentation_show_credits_legal_images(buffer, capacity);
    free(buffer);
    disp_prepare(2);
    /* Restart the frame after fullscreen images switched banks */
    disp_clear_ordering_table();
    presentation_begin_frame();
    x = disp_options.scr_x;
    y = disp_options.scr_y;
    disp_state.frames[0].disp.screen.x = (sint16)(x);
    disp_state.frames[1].disp.screen.x = (sint16)(x);
    disp_state.frames[0].disp.screen.y = (sint16)(y);
    disp_state.frames[1].disp.screen.y = (sint16)(y);
    resume_menu_music();
    audio_stop_voice(20);
    session_pop_menu();
}

static void advance_credits(void)
{
    uint32 idx, val, cursor, image_idx;
    const char *pointer, *text;
    uint16 *pos;
    sint32 y;

    FUNCTION_MARKER(0x800A7674, "SLUS_006.10");
    val = credits.viewport_flags;
    presentation_items[0].x_flags = (val);
    presentation_select_pal(2);
    for (idx = 0; idx < 11; ++idx)
    {
        pointer = credits.rows[idx].text;
        val = (uint8)*pointer;
        if (val == 'c' || val == 'd')
        {
            presentation_select_pal(3);
            y = (sint16)(credits.rows[idx].y);
            pointer = credits.rows[idx].text;
            presentation_draw_text(130, 256, y, pointer);
            presentation_select_pal(2);
        }
        else
        {
            y = (sint16)(credits.rows[idx].y);
            presentation_draw_text(128, 256, y, pointer);
        }
    }
    if ((presentation_menu.frame_counter & 1) != 0)
        return;
    for (idx = 0; idx < 11; ++idx)
    {
        pos = &credits.rows[idx].y;
        y = (sint16)*pos;
        if (y >= -23)
            *pos = ((uint32)y - 1);
        else
        {
            cursor = credits.cursor;
            *pos = 240;
            if (credits_lines[cursor + 1] != NULL)
            {
                text = credits_lines[cursor];
                credits.rows[idx].text = text;
                text = credits.rows[idx].text;
                credits.cursor = (uint32)(cursor + 1);
                if ((uint8)*text == 'a')
                {
                    image_idx = credits.image_idx;
                    credits.rows[idx].text = text + 1;
                    upload_credit_image(image_idx);
                    val = credits.image_idx;
                    credits.image_idx = (uint32)(val + 1);
                }
                text = credits.rows[idx].text;
                if ((uint8)*text == 'e')
                {
                    text = credits.rows[idx].text;
                    credits.viewport_flags = (uint32)(768);
                    credits.rows[idx].text = text + 1;
                }
                text = credits.rows[idx].text;
                if ((uint8)*text == 'b')
                    restore_credits_background();
            }
            else
            {
                credits.rows[idx].text = credits_lines[0];
            }
        }
    }
}

void credits_update(void)
{
    FUNCTION_MARKER(0x800A7884, "SLUS_006.10");
    advance_credits();
    if ((sint16)input_ports[0].pressed != 0)
        restore_credits_background();
}
