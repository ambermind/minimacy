syntax keyword language hide endif elifndef elifdef ifndef as ifdef export proto \integer \bigNum \mod \modBarrett \float use include struct sum enum extend let set of in break return throw catch try with match lambda fun do else then if for while nil call const var

syntax keyword definitions regexpTest regexpNext regexpCreate RegExp jsonList jsonListGet jsonInt jsonBool Json jsonNumber jsonString jsonNull jsonTrue jsonFalse jsonArray jsonObject jsonNumberGet jsonIntGet jsonBoolGet jsonStringGet jsonObjectGet jsonArrayGet jsonFieldGet jsonParse jsonEncode jsonEncodePretty dnsSetServer dnsAddHost dnsRequest isNan isInf tanh tan sqrt sqr sinh sin round pi minf maxf log ln ceil atan atan2 asin acos absf Floats Float cosh cos e exp floor floatsLength floatsGet floatsFromArray strFromFloat intFromFloat strFloat zipFileName zipFlag zipCompressed zipUncompressed zipContent zipUpdate zipLoadContent zipExtractFiles dosTimeStamp dosTimeFromTime zipFromFileList zipFromFileInfoList ZipFile mkFont arrayReduce arraySlice listFromArray arrayMapIndex arrayMap arrayLength arrayInit arrayIndexOf arrayCreate array bitmapPolygon bitmapPolygons deflateBytes deflate inflate lzwFromStr strFromLzw lzwInflate lzwDeflateFinalize lzwDeflate lzwCreate Lzw SSHshell stateSH shellParseChannelSuccess shellParseChannelData shellHandle shellStart shellConnect gifFromBitmap gifFromFrames listFromHashmap hashmapTest hashmapSet hashmapMap hashmapInit hashmapGet hashmapFind hashmapCreate hashmapCount hashmap httpGetHeaders httpGetHeader httpSetTimeout httpForceTls12 httpSetUrlEncoded httpGet12 httpSetHeader httpGetRemote httpFormUrlEncodedFromList httpReuse httpCreate httpSetPostData httpSetCipherSuites httpSetClientCertificate HttpArgs stringArg fileArg httpSetMultipart httpSend httpGet shamirMakeSecret shamirFiniteField shamirFromSecret shamirSolve inputs partitions fs pkg pkgScope pkgList start dir reboot dumpCallstack timers sys help consoleStart des3Key192 des3Encrypt des3Decrypt des3EncryptEcb des3DecryptEcb des3EncryptCbc des3DecryptCbc signExtend8 signExtend32 signExtend24 signExtend16 signExtend range min max bitTest abs hexFromInt floatFromInt decNFromInt strFromInt decFromInt dec3FromInt intAbbrev intRand Int glTexSubImage2D glTexImage2DUpdate glViewport glVertexAttribPointer glUseProgram glUniformMatrix4fv glUniformMatrix3fv glUniformMatrix2fv glUniform4fv glUniform3fv glUniform2fv glUniform1fv glTexParameteri glTexParameterf glTexImage2D glSwapBuffers glShaderSource glScissor glLinkProgram glLineWidth glHint glGetUniformLocation glGetString glGetAttribLocation glFrontFace glFlush glEnableVertexAttribArray glEnable glDrawArrays glDisableVertexAttribArray glDisable glDepthMask glDepthFunc glCullFace glCreateTexture glCreateShader glCreateProgram glCopyTexSubImage2D glCopyTexImage2D glCompileShader glClearColor glClear glBlendFunc glBindTexture glAttachShader glActiveTexture GlTexture GlShader GlProgram GL_ZERO GL_VERTEX_SHADER GL_VERSION GL_VENDOR GL_TRUE GL_TRIANGLE_STRIP GL_TRIANGLE_FAN GL_TRIANGLES GL_TEXTURE_WRAP_T GL_TEXTURE_WRAP_S GL_TEXTURE_MIN_FILTER GL_TEXTURE_MAG_FILTER GL_TEXTURE_2D GL_TEXTURE0 GL_STATIC_DRAW GL_SRC_COLOR GL_SRC_ALPHA_SATURATE GL_SRC_ALPHA GL_SHADING_LANGUAGE_VERSION GL_SCISSOR_TEST GL_RGBA GL_RGB GL_REPLACE GL_REPEAT GL_RENDERER GL_POINTS GL_ONE_MINUS_SRC_COLOR GL_ONE_MINUS_SRC_ALPHA GL_ONE_MINUS_DST_COLOR GL_ONE_MINUS_DST_ALPHA GL_ONE GL_NOTEQUAL GL_NICEST GL_NEAREST_MIPMAP_NEAREST GL_NEAREST_MIPMAP_LINEAR GL_NEAREST GL_LUMINANCE GL_LINE_STRIP GL_LINE_LOOP GL_LINES GL_LINEAR_MIPMAP_NEAREST GL_LINEAR_MIPMAP_LINEAR GL_LINEAR GL_LESS GL_LEQUAL GL_GREATER GL_GEQUAL GL_FRONT_AND_BACK GL_FRONT GL_FRAMEBUFFER GL_FRAGMENT_SHADER GL_FASTEST GL_FALSE GL_EXTENSIONS GL_EQUAL GL_DST_COLOR GL_DST_ALPHA GL_DONT_CARE GL_DITHER GL_DEPTH_TEST GL_DEPTH_BUFFER_BIT GL_CW GL_CULL_FACE GL_COLOR_BUFFER_BIT GL_CLAMP_TO_EDGE GL_CCW GL_BLEND GL_BACK GL_ARRAY_BUFFER GL_ALWAYS glES bytesCmp bytesCopyBytes bytesFromStr bytesRight bytesLeft bytesBuild bytesXor bytesXorBytes bytesWriteVarUInt bytesWriteVarInt bytesWrite8 bytesWrite32Msb bytesWrite32Lsb bytesWrite24Msb bytesWrite24Lsb bytesWrite16Msb bytesWrite16Lsb bytesVarUIntNext bytesVarIntNext bytesSwapStr bytesSet bytesReadVarUInt bytesReadVarInt bytesRead8 bytesRead32Msb bytesRead32Lsb bytesRead24Msb bytesRead24Lsb bytesRead16Msb bytesRead16Lsb bytesRand bytesLength bytesLSL1 bytesInit bytesGet bytesSliceOfStr bytesSlice bytesCreate bytesCopy bytesClear Bytes MODE_NUMERIC MODE_ALPHANUMERIC MODE_BYTE ECL_LOW ECL_MEDIUM ECL_QUARTILE ECL_HIGH qrcodeEcho qrcode qrcodeWithBorder run msgException anyException Exception equals rsaPssVerify rsaPssSign Sample soundStart soundAbort soundPlaying cursorSize cursorCreate cursorShow Cursor onInput onPrompt orientationGet accelerometerZ accelerometerY accelerometerX accelerometerInit keyboardHeight keyboardHide keyboardShow clipboardPaste clipboardCopy screenW screenH uiOnDropFiles uiOnResume uiOnSuspend uiMultiTouch uiW uiUpdateRegion uiUpdate uiStop uiStart uiSetTitle uiResize uiOnVwheel uiOnUnclick uiOnSize uiOnPaint uiOnMouseMove uiOnKeyUp uiOnKeyDown uiOnHwheel uiOnClose uiOnClick uiH uiBuffer keyboardState UI_RESIZE UI_NORMAL UI_GL UI_FULLSCREEN Key_Up Key_Sys_Req Key_Scroll_Lock Key_Right Key_Prior Key_Pause Key_Page_Up Key_Page_Down Key_Num_Lock Key_Next Key_Left Key_Insert Key_Home Key_F9 Key_F8 Key_F7 Key_F6 Key_F5 Key_F4 Key_F3 Key_F2 Key_F1 Key_F12 Key_F11 Key_F10 Key_End Key_Down Key_Delete Key_Caps_Lock KeyMask_Shift KeyMask_Meta KeyMask_Control KeyMask_Alt quicksortFusion udpSend udpSendTo udpRemotePort udpRemoteIp udpOnEvent udpCreate udpClose nameByIp ipByName hostName tcpWrite tcpSrvClose tcpRemotePort tcpRemoteIp tcpOpen tcpNoDelay tcpSrvOnAccept tcpSrvCreate tcpClose tcpOnEvent Udp TcpSrv Tcp rsaPublicFromCertificate rsaKey rsaFromPEM rsaFromKey publicKeyFromCertificate keyIsPrivatep keyFromPEM keyDump ecPublicFromCertificate ecPointFromAsn1 ecKeyFromPEM ecKey ecFromKey ecCurveFromOID asn1RsaKeyPub asn1RsaEncryption asn1KeyEncryption asn1EcPoint Key strUppercaseU8 strUppercaseU16Le strUppercaseU16Be strUnaccentedU8 strUnaccentedU16Le strUnaccentedU16Be strUnaccented strSearchcaseU8 strSearchcaseU16Le strSearchcaseU16Be strSearchcase strLowercaseU8 strLowercaseU16Le strLowercaseU16Be strFromBytes strCreate strFormat strEmpty strLeftSpacePadding strWithCRLF strWithCR strWithLF hexFromStr u8FromXml latinFromXml trim strFromSource u8FromLatin u16BeFromLatin u16LeFromLatin u8FromJson isU8 intFromHex strFromHex hexFilter strFromChar strFromUrl u16BeFromU8 u16LeFromU8 latinFromU8 jsonFromU8 u8FromU16Be u8FromU16Le latinFromU16Be latinFromU16Le strVarUIntNext strVarUInt strVarIntNext strVarInt strUppercase xmlFromStr urlFromStr sqlFromStr sourceFromStr floatFromStr strSwap strSliceOfBytes strSlice strStartsWith strLines strSplit strRight strReplace strReadFloat bytesReadFloat strReadVarUInt strReadVarInt strU8Next strU8Previous strReadU8 strRead8 strRead32Msb strRead32Lsb strRead24Msb strRead24Lsb strRead16Msb strRead16Lsb strRand strCheckPos strPosRev strPos strLowercase strListConcat strLengthU8 strLengthU16 strLength strLeft strLeftBytes strJoin strInt8 strInt32Msb strInt32Lsb strInt24Msb strInt24Lsb strInt16Msb strInt16Lsb strGet strEndsWith strContains strCmp strCharPosRev strCharPos strConcat strBuild intFromDec Str rsaRandomMsg rsaPubExp rsaModulus rsaIsPrivate rsaFromPublic rsaFromPQ rsaDump rsaCreate rsaComputePrivateExp qR qInvR pubExpR privExpR pR modulusR dQR dPR RSA nativeFonts nativeFontW nativeFontBaseline nativeFontH nativeFontCreate nativeFontDraw FONT_UNDERLINE FONT_STRIKED FONT_ITALIC FONT_BOLD NativeFont hashsetCreate hashsetContains hashsetAdd hashsetRemove hashsetCount hashsetFind hashsetMap listFromHashset hashsetTest hashsetInit hashset rsaPkcs1SignatureCheck rsaPkcs1Sign pkcs1EncryptPubRsa pkcs1EncryptPub pkcs1EncryptPrivRsa pkcs1EncryptPriv pkcs1DecryptPubRsa pkcs1DecryptPub pkcs1DecryptPrivRsa pkcs1DecryptPriv refSet refGet refCreate Ref Tree1D tree1DDepth tree1DInsert tree1DRemove tree1DVisit tree1DVisitDesc tree1DVisitRange tree1DContains tree1DIndex tree1DRow Transfer transferLength transferChunked transferNone HTTPCLIENT_TIMEOUT WEBSOCKET_TIMEOUT HTTP_1_0 HTTP_1_1 HTTP_CONTENT_LENGTH HTTP_CONTENT_TYPE HTTP_CACHE_CONTROL HTTP_LOCATION HTTP_COOKIE HTTP_SET_COOKIE HTTP_TRANSFER_ENCODING HTTP_CHUNKED HTTP_URLENCODED HTTP_MULTIPART WS_KEY WS_ACCEPT WS_UUID WS_OPCODE_CONTINUE WS_OPCODE_TEXT WS_OPCODE_BIN WS_OPCODE_CLOSE WS_OPCODE_PING WS_OPCODE_PONG oblivionCreate Oblivion aesCheckLength aesEncryptCbc aesDecryptCbc aesEncryptEcb aesDecryptEcb aesEncryptCtr aesEncryptCtrIncr aesCmac aesGcmEncrypt aesGcmDecrypt aesGcmDecryptGroup leafH nodeH BitWriter bwCreate bwAlign bwFinal bwBytes bwBitsLsb bwBitsMsb bwBitsLsbInv BitReader brCreate brBitsLsb brBitsMsb brBit0To7 brBit7To0 brBytes HuffmanTree huffmanDumpEx huffmanDump huffmanDumpList huffmanCountStr huffmanCountAdd huffmanComputeCodeLengths huffmanTreeFromFifos huffmanBuildDecoder huffmanBuildEncoder huffmanEncodeCode0To7 huffmanEncodeStr0To7 huffmanEncodeCode7To0 huffmanDecodeCode0To7 huffmanDecodeCode7To0 Tree23 tree23Insert tree23Delete tree23Visit tree23VisitDesc tree23Find tree23AtLeast tree23Depth tree23Dump ecbEncryptIntoBytes ecbDecryptIntoBytes ecbEncrypt ecbDecrypt CBC cbcCreate cbcEncryptIntoBytes cbcDecryptIntoBytes cbcEncrypt cbcDecrypt ctrEncrypt ctrEncryptRef ctrEncryptOpti gcmProcess gcmEncrypt gcmDecrypt paddingZero paddingANSI_X9_23 paddingISO_10126 paddingPKCS_5 paddingISO_7816_4 paddingTLS unPaddingANSI_X9_23 unPaddingISO_10126 unPaddingPKCS_5 unPaddingTLS unPaddingISO_7816_4 listCut listPosition doubleListCompress listVisit tripleListCompress tail listTest listReverse listRemove listReduce quicksort listMap listFind listInsert listFilter listContains head arrayFromList listMatchHead listLength listLast listGet listConcat list bytesAdler32 strAdler32 bytesCrc32 strCrc32 sha512Process sha512ProcessBytes sha512Output sha512Create sha384Process sha384ProcessBytes sha384Output sha384Create sha256Process sha256ProcessBytes sha256Output sha256Create sha1ProcessBytes sha1Process sha1Output sha1Create rc4WriteBytes rc4Output rc4Create md5Process md5ProcessBytes md5Output md5Create Md5 desWriteBytes desOutput desEncrypt desDecrypt desCreate aesWriteBytes aesOutput aesEncrypt aesEncryptBytes aesDecrypt aesDecryptBytes aesCreate Sha512 Sha384 Sha256 Sha1 Rc4 Des DES_BLOCK Aes AES_BLOCK strFromOid oidHashmapSubjectAltName oidHashmapSUBJECT oidHashmapSIGNATURE oidHashmapSIGNALGO oidHashmapPUBKEY oidHashmapPUBEXP oidHashmapMODULUS oidHashmapISSUER oidHashmapINFO oidHashmapENCRYPTION oidFromStr niceOID echoOIDHashmap OID_subjectKeyIdentifier OID_subjectAltName OID_serverAuth OID_serialNumber OID_secp384r1 OID_rsaEncryption OID_prime256v1 OID_organization_validated OID_keyUsage OID_jurisdictionOfIncorporationStateOrProvinceName OID_jurisdictionOfIncorporationCountryName OID_id_ad_ocsp OID_id_ExtensionReq OID_extKeyUsage OID_ev_guidelines OID_emailAddress OID_ecPublicKey OID_domain_validated OID_cps OID_clientAuth OID_certificatePolicies OID_caIssuers OID_cRLDistributionPoints OID_businessCategory OID_basicConstraints OID_authorityKeyIdentifier OID_authorityInfoAccess OID_SUBJECT OID_ST OID_SIGNATURE OID_SIGNALGO OID_SEP OID_RSA_PKCS1_SHA512 OID_RSA_PKCS1_SHA384 OID_RSA_PKCS1_SHA256 OID_RSA_PKCS1_SHA1 OID_PUBKEY OID_PUBEXP OID_OU OID_O OID_MODULUS OID_L OID_ISSUER OID_INFO OID_ENCRYPTION OID_CURVE OID_CN OID_C OID_2_5_4_8 OID_2_5_4_7 OID_2_5_4_6 OID_2_5_4_5 OID_2_5_4_3 OID_2_5_4_15 OID_2_5_4_11 OID_2_5_4_10 OID_2_5_29_37 OID_2_5_29_35 OID_2_5_29_32 OID_2_5_29_31 OID_2_5_29_19 OID_2_5_29_17 OID_2_5_29_15 OID_2_5_29_14 OID_2_23_140_1_2_2 OID_2_23_140_1_2_1 OID_2_23_140_1_1 OID_2_16_840_1_114412_2_1 OID_2_16_840_1_114412_1_1 OID_1_3_6_1_5_5_7_48_2 OID_1_3_6_1_5_5_7_48_1 OID_1_3_6_1_5_5_7_3_2 OID_1_3_6_1_5_5_7_3_1 OID_1_3_6_1_5_5_7_2_1 OID_1_3_6_1_5_5_7_1_1 OID_1_3_6_1_4_1_311_60_2_1_3 OID_1_3_6_1_4_1_311_60_2_1_2 OID_1_3_6_1_4_1_311_42_1 OID_1_3_6_1_4_1_311_21_7 OID_1_3_6_1_4_1_311_21_10 OID_1_3_6_1_4_1_11129_2_5_3 OID_1_3_6_1_4_1_11129_2_4_2 OID_1_3_132_0_34 OID_1_2_840_113549_1_9_14 OID_1_2_840_113549_1_9_1 OID_1_2_840_113549_1_1_5 OID_1_2_840_113549_1_1_13 OID_1_2_840_113549_1_1_12 OID_1_2_840_113549_1_1_11 OID_1_2_840_113549_1_1_1 OID_1_2_840_10045_4_3_3 OID_1_2_840_10045_3_1_7 OID_1_2_840_10045_2_1 NiceOIDs fifoOut fifoNext fifoList fifoIn fifoCreate fifoCount fifo fifoFromList subjectFromAsn1 strFromAsn1 rsaPubFromAsn1 listFromAsn1Subject listFromAsn1 intFromAsn1 derProcess commonFromAsn1 boolFromAsn1 asn1Utf8String asn1UtcTime asn1Unpack asn1Subject asn1Set asn1Seq asn1Raw asn1PrintableString asn1Pack asn1OctetString asn1ObjectIdentifier asn1Null asn1MakeInfos asn1ListOfStr asn1ListOfRecStr asn1ListOfNiceOid asn1ListOfHex asn1ListOfBignum asn1Integer asn1InfoBlock asn1IA5String asn1GetObject asn1Else asn1Echo asn1DistinguishedName asn1ClassPrimitive asn1ClassConstructed asn1Bool asn1BitString Asn1 ASN1_UTF8STRING ASN1_UTCTIME ASN1_UNIVERSAL ASN1_SET ASN1_SEQUENCE ASN1_PRIVATE ASN1_PRINTABLESTRING ASN1_OCTETSTRING ASN1_OBJECTIDENTIFIER ASN1_NULL ASN1_INTEGER ASN1_IA5STRING ASN1_EXTERNAL ASN1_CONTEXT_SPECIFIC ASN1_BOOLEAN ASN1_BITSTRING ASN1_APPLICATION marshalMaker jpgFromBitmap tokenCreate tokenNext Token TOKEN_BACKSLASH TOKEN_QUOTE TOKEN_DOUBLEQUOTE TOKEN_SPACES TOKEN_NUMERIC TOKEN_HEXA TOKEN_ALPHANUM_UNDER tokenDone tokenIndex tokenContextError tokenMoveOn tokenTake tokenCutAll tokenFilterSkip tokenFilterKeywords tokenFilterCommentLine tokenFilterComment tokenFilterUntil tokenFilterHexa tokenFilterInteger tokenFilterFloat tokenFilterWord tokenFilterString tokenFilterNumber tokenFilterAnyChar tokenIsNumber tokenIsWord insertsort bigX25519 x25519 x25519Random x25519KeyPair x25519KeyPub x25519Ecdh bitmapFromJpg tablePrettyPrint Sprite gl2dDrawBackground gl2dInitScreen gl2dDrawFlatBox gl2dSpriteCreate gl2dSpriteUpdateUV gl2dSpriteUpdateTexture gl2dDrawColoredSprite gl2dDrawSprite BLEND_DESTINATION yCrCbFromRgb bitmapToYCrCb dct88 rgbFromYCrCb bitmapFromYCrCb idct88 Component COMP_255 COMP_0 COMP_R_INV COMP_R COMP_G_INV COMP_G COMP_B_INV COMP_B COMP_A_INV COMP_A BLEND_XOR BLEND_SUB BLEND_OR BLEND_NONE BLEND_MUL BLEND_MIN BLEND_MAX BLEND_AND BLEND_ALPHA BLEND_ADD BlendFunction Bitmap bitmapW bitmapColoredBlit bitmapBlit bitmapSet bitmapScanline bitmapResize bitmapRectangle bitmapPlot bitmapMakeColorTransparent bitmapLine bitmapH bitmapGet bitmapFillRectangle bitmapFillCircle bitmapFill bitmapErase bitmapCreate bitmapCopy bitmapComponents bitmapCircle Texture glT wT hT bmpT GL_PROJECTION GL_MODELVIEW GL_PROJECTION_MATRIX GL_MODELVIEW_MATRIX glInitShaders glPushMatrix glPopMatrix glLoadIdentity glMatrix glMatrixMode glFrustum glOrtho glTranslate glScale glRotate glMultMatrix glProject3d renderFlat renderColors renderColors1Light renderColors2Lights renderTexture colorMakeARGB textureCreate textureUpdateRegion textureUpdate textureComputeRectangleUV mimeFromFilename tlsTcpOpenExt tlsTcpOpen tlsTcpAccept tlsFromStream tlsMakeHosts tlsSetCipherSuites tlsSetSignatureAlgorithms tlsSetSupportedGroups tlsGetClientCertificate tlsAlert tlsWho tlsGetServerName TLS_AES_128_GCM_SHA256 TLS_AES_256_GCM_SHA384 TLS_x25519 TLS_secp256r1 TLS_secp384r1 TLS_secp521r1 tlsStage TLS13 sha256 sha384 sha512 sha1 md5 hmacSha512 hmacSha384 hmacSha256 hmacSha1 hmacMd5 csrPEM csrFromPEM csrFromDER csrDER EllipticCurve EcKey ecSecp256k1 ecSecp256r1 ecSecp384r1 ecSecp521r1 ecCurve25519 ecMul ecAdd ecTest ecName ecRandom ecDump ecStrFromPoint ecPointFromStr ecKeyDump ecKeyFromPublic ecKeyFromPrivate ecCreateKey ecKeyPub ecKeyPubStr ecIsPrivate ecEcdh ecEcdhStr ecSign ecVerify randomHardware randomEntropy setHostUser hardStop niceStop version device args kill await appStart Thread exit threadT0 threadStop threadPost threadName threadId threadStart threadCycles threadOrigin thisThread joinWait joinSend joinCreate Join lockCreate lockSync Lock fileDelete append load userDir save cleanPath FileMode FileInfo File FILE_REWRITE FILE_READ_WRITE FILE_READ_ONLY FILE_APPEND dirList fileList diskListInfo dirListInfo fileListInfo fileTell fileWrite fileSize fileRead fileOpen fileInfoUpdate fileInfoSize fileInfoName fileInfoIsDir fileInfo fileClose parentDir SOURCE_EXTENSION gzipFromStr strFromGzip tarLoadContent TarFile tarMode tarFileName tarSize tarContent tarUpdate tarExtractDirectories tarExtractFiles tarFromFileInfoList tarFromFileList ERROR_PACKET ERROR_CLOSED ERROR_BUSY ERROR_CONNECT MysqlReply errorReply okReply rowReply Mysql mysqlConnect mysqlSetCharset mysqlQuery mysqlUseDatabase mysqlQuit mysqlProtocol mysqlVersion mysqlColumns mysqlLastInsert sql mysqlConvertColumns HttpSrv httpSrvUpdateHostList httpSrvStartEx httpSrvSetDefaultTimeout httpSrvStop httpSrvOnRequest httpSrvOnWebSocket httpSrvOnMessage httpSrvGetRequestHeader httpSrvGetServerName httpSrvSetTimeout httpSrvStart httpSrvGetRemote httpSrvAddHeader httpSrvSetHeader httpSrvRedirect httpSrvSetCookie httpSrvGetCookies httpSrvGetCookie httpSrvGetRequest httpSrvGetRequestHeaders httpSrvGetPostData httpSrvSetStatus httpSrvGetClientCertificate httpSrvGetClientCertificateLogin httpSrvParseArgs httpWsSend httpWsClose HttpConnection COOKIE_SECURE COOKIE_HTTP_ONLY COOKIE_SAMESITE_LAX COOKIE_SAMESITE_STRICT COOKIE_SAMESITE_NONE SSH_DEBUG SshAuth sshAuthPassword sshAuthPublicKey SSH_CODE SSH_OK SSH_NO_ALGO SSH_SIGNATURE SSH_USERAUTH_FAILURE SSH_CHANNEL_FAILURE SSH_UNKNOWN SSH_HMAC SSH_CHANNEL_EOF SSH_CLOSED SSH_NOK SSH_CLIENT SSH2_MSG_DISCONNECT SSH2_MSG_IGNORE SSH2_MSG_SERVICE_REQUEST SSH2_MSG_SERVICE_ACCEPT SSH2_MSG_KEXINIT SSH2_MSG_NEWKEYS SSH2_MSG_KEX_ECDH_INIT SSH2_MSG_KEX_ECDH_REPLY SSH2_MSG_KEX_DH_GEX_GROUP SSH2_MSG_KEX_DH_GEX_INIT SSH2_MSG_KEX_DH_GEX_REPLY SSH2_MSG_KEX_DH_GEX_REQUEST SSH2_USERAUTH SSH2_MSG_USERAUTH_REQUEST SSH2_MSG_USERAUTH_FAILURE SSH2_MSG_USERAUTH_SUCCESS SSH2_MSG_USERAUTH_BANNER SSH2_MSG_USERAUTH_PK_OK SSH2_MSG_GLOBAL_REQUEST SSH2_MSG_REQUEST_SUCCESS SSH2_MSG_REQUEST_FAILURE SSH2_MSG_CHANNEL_OPEN SSH2_MSG_CHANNEL_OPEN_CONFIRMATION SSH2_MSG_CHANNEL_OPEN_FAILURE SSH2_MSG_CHANNEL_WINDOW_ADJUST SSH2_MSG_CHANNEL_DATA SSH2_MSG_CHANNEL_EOF SSH2_MSG_CHANNEL_CLOSE SSH2_MSG_CHANNEL_REQUEST SSH2_MSG_CHANNEL_SUCCESS SSH2_MSG_CHANNEL_FAILURE SSH_MIN SSH_N SSH_MAx CLIENT_PACKET_MAX_SIZE CLIENT_WINDOW_INIT SSH_ZERO_PADDING BLOCK_SIZE HMAC_FAILED OID_SHA1 OID_SHA256 OID_SHA512 SshStage greetingS packetsS SshDhgex pDH gDH xDH eDH fDH kDH hDH ecDH QsDH hostKeyDH signDH SshKeys encIvK decIvK encKeyK decKeyK encMacK decMacK decMacLenK gcmEncK gcmDecK encEnabledK decEnabledK kex_algorithms server_host_key_algorithms encryption_algorithms_client_to_server encryption_algorithms_server_to_client mac_algorithms_client_to_server mac_algorithms_server_to_client compression_algorithms_client_to_server compression_algorithms_server_to_client languages_client_to_server languages_server_to_client Mandatory SshInit cookieI algosI sequenceNumberI SSH tcpS sendS iS closeAfterS stageS headerS clientInitS srvInitS dhgexS hashDataS keysS publicKeyS fHandlePacketS cbResultS serverPacketMaxSizeS serverWindowS sendingS clientPacketMaxSizeS clientWindowS sshClientInit sshEchoLn sshDump sshHexDump sshMakePacket sshMsgStr sshMsgRaw sshMsgChar sshMsgInt sshMsgInt64 sshMakeMsg sshMakeKeyExchangeInit sshMakeDhgeRequest sshMakeEcdhRequest sshMakeDhgeInit sshMakeNewKeys sshTcpClose sshParseGreeting sshParseVals sshNotify sshCodeUnknown sshFail sshParseKeyExchangeInit sshParseKexDhgeGroup sshParseKexDhgeReply sshParseKexEcdhReply sshSendCrypt sshCheckSign sshParseNewKeys sshParseServiceAccept sshParseUserAuthPkOk sshParseUserAuthFailure sshParseUserAuthSuccess sshParseChannelOpenConfirmation sshParseChannelWindowAdjust sshParseChannelSuccess sshParseChannelFailure sshParseChannelEof sshParseChannelClose sshParseChannelRequest sshParseGlobalRequestDefault sshDecodePacket sshHandleDefault sshHandleConnectEcdh sshHandleConnect sshHandleAuth sshParsePacket sshParse sshMakeCb sshAuth sshAuthWithPassword sshAuthWithPublicKey sshSendMsg sshSendData sshConnectAndAuth sshConnectWithPublicKey sshConnectWithPassword sshConnect sshClose ALIGN_LEFT ALIGN_RIGHT ALIGN_CENTER ALIGN_TOP ALIGN_BOTTOM ALIGN_BASELINE ALIGN_MIDDLE Font fontFromBitmap fontFixedFromBitmap fontCharWidth fontH fontStringMultiline fontStringMultilineU8 fontStringMultilineHeight fontStringMultilineHeightU8 fontStringWidth fontStringWidthU8 fontPosition fontPositionU8 bitmapText bitmapTextU8 bitmapMultiLineText bitmapMultiLineTextU8 BMP_24 BMP_32 bmpFromBitmap bitmapFromBmp systemLogEnable echoTime echoLn echoHexLn echoEnable echo binDump bigDump hexDump dump bufferSliceOfBytes bufferSliceOfStr bufferGet bufferAppendChar strFromBuffer bytesFromBuffer bufferReset bufferLength bufferCreateWithSize bufferCreate bufferAppendJoin bufferAppend Buffer htmlParse tagXml contentXml Xml xmlEncodePretty xmlEncode xmlParse xmlGetTag xmlGetAttributes xmlGetChilds xmlGetContent xmlGetAttribute xmlFilterChilds quantifyFrame paletteFromQuantization paletteProject bitmapPalettize quantization bigPrimal Days Months timeFromDate time smallDate sleepMs onTimeout fileDateSeconds fileDate fullDate smallTime date timeFromYyddmmhhmmssz yyddmmhhmmsszFromTime timeMs T0 ed25519secret_to_public ed25519sign ed25519verify pemRead pemDecode pemMake bitmapFromPng b64Encode b64Decode urlFromB64 b64FromUrl pngFromBitmap oaepEncryptPub oaepDecryptPub oaepEncryptPriv oaepDecryptPriv oaepEncryptPubRsa oaepDecryptPubRsa oaepEncryptPrivRsa oaepDecryptPrivRsa rootCertificateRegister cerMatchServerName cerKeyIsRSA cerKeyIsEC cerFromPEM cerFromDER cerFromAsn1 cerEcho cerDefaultVersion cerCheckTime cerCheckChain cerCheckByAuth cerByServerName gifAnimate framesFromGif bitmapFromGif bigXor signedStrFromBig intFromBig hexFromBig decFromBig strFromBig bigSubMod bigSub bigRand bigPower2 bigPositive bigGcd bigNegMod bigNeg bigNbits bigMulModBarrett bigMulMod bigMul bigModPower2 bigModBarrett bigMod bigLowestBit bigLowerEquals bigLower bigIsOne bigIsNull bigIsEven bigInv bigGreaterEquals bigGreater bigFromSignedStr bigFromSignedBytes bigFromInt bigFromHex bigFromDec bigFromStr bigFromBytes bigExpModBarrett bigExpMod bigExpChinese bigExpChinese7 bigExpChinese5 bigExp bigEuclid bigEquals bigDivRemainder bigDivModBarrett bigDivMod bigDiv bigCmp bigBit bigBarrett bigAddMod bigAdd bigAbs bigASR bigASR1 bigASL1 BigNum SFTP_VERSION SSH_FXP_INIT SSH_FXP_VERSION SSH_FXP_OPEN SSH_FXP_CLOSE SSH_FXP_READ SSH_FXP_WRITE SSH_FXP_LSTAT SSH_FXP_FSTAT SSH_FXP_SETSTAT SSH_FXP_FSETSTAT SSH_FXP_OPENDIR SSH_FXP_READDIR SSH_FXP_REMOVE SSH_FXP_MKDIR SSH_FXP_RMDIR SSH_FXP_REALPATH SSH_FXP_STAT SSH_FXP_RENAME SSH_FXP_READLINK SSH_FXP_SYMLINK SSH_FXP_STATUS SSH_FXP_HANDLE SSH_FXP_DATA SSH_FXP_NAME SSH_FXP_ATTRS SSH_FXP_EXTENDED SSH_FXP_EXTENDED_REPLY SSH_FX_OK SSH_FX_EOF SSH_FX_NO_SUCH_FILE SSH_FX_PERMISSION_DENIED SSH_FX_FAILURE SSH_FX_BAD_MESSAGE SSH_FX_NO_CONNECTION SSH_FX_CONNECTION_LOST SSH_FX_OP_UNSUPPORTED SSH_FXF_READ SSH_FXF_WRITE SSH_FXF_APPEND SSH_FXF_CREAT SSH_FXF_TRUNC SSH_FXF_EXCL SSH_FILEXFER_ATTR_SIZE SSH_FILEXFER_ATTR_UIDGID SSH_FILEXFER_ATTR_PERMISSIONS SSH_FILEXFER_ATTR_ACMODTIME SSH_FILEXFER_ATTR_EXTENDED SftpCallback fileListS handleS errorS fileContentS attrS okS SftpSort SORT_BY_NAME SORT_BY_NAME_DESC SORT_BY_SIZE SORT_BY_SIZE_DESC SORT_BY_MODIFICATION_TIME SORT_BY_MODIFICATION_TIME_DESC SORT_BY_ACCESS_TIME SORT_BY_ACCESS_TIME_DESC Sftp countSF cbSF homeSF offsetSF pendingDataSF sendingDataSF sendingOffsetSF SftpFile shortF longF sizeF uidF gidF permissionsF atimeF mtimeF extendedF sftpSend sftpParseChannelSuccess sftpParseVersion sftpParseName sftpParseAttrs sftpParseHandle sftpParseStatus sftpParseData sftpParseChannelData sftpHandleFtp sftpFtpStart sftpConnectAsync sftpDirAsync sftpMkdirAsync sftpRmdirAsync sftpGetAsync sftpStatAsync sftpLStatAsync sftpPutAsync sftpSetStatAsync sftpRenameAsync sftpRemoveAsync sftpIsDir sftpEchoDir sftpAttrFromPermission sftpHome sftpSort sftpConnect sftpDir sftpMkdir sftpRmdir sftpGet sftpStat sftpLStat sftpPut sftpSetStat sftpRename sftpRemove sftpMkPath sftpWWWSend strFromBool true false Bool WS_OPCODE_TEXT wsCreate12 wsGetRemote wsGetStatus wsGetReplyHeaders wsCreate wsConnect wsSend wsClose WebSocket signatureGenerate signatureForRsa signatureForEc signatureCheckFromCertificate signatureCheck signOIDByAlgo signAlgoByOID RSA_PSS_RSAE_SHA512 RSA_PSS_RSAE_SHA384 RSA_PSS_RSAE_SHA256 RSA_PKCS1_SHA512 RSA_PKCS1_SHA384 RSA_PKCS1_SHA256 RSA_PKCS1_SHA1 ECDSA_SECP384R1_SHA384 ECDSA_SECP256R1_SHA256 ANS1_SHA512_MAGIC ANS1_SHA384_MAGIC ANS1_SHA256_MAGIC ANS1_SHA1_MAGIC

syntax match other '\w*'
syntax match operators '[~#;=<>!>,:&/%\.\^\*\+\-\?\(\)\[\]\{\}\\\|0123456789]'
syntax match comment "//.*$"
syntax region string start='"' end='"'
syntax region comment start="/\*" end="\*/"
