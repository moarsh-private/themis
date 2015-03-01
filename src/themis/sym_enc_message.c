/**
 * @file
 *
 * (c) CossackLabs
 */

#include <common/error.h>
#include <themis/sym_enc_message.h>
#include <soter/soter.h>
#include <string.h>


#define THEMIS_SYM_KEY_LENGTH SOTER_SYM_256_KEY_LENGTH
#define THEMIS_AUTH_SYM_ALG (SOTER_SYM_AES_GCM|THEMIS_SYM_KEY_LENGTH)
#define THEMIS_AUTH_SYM_IV_LENGTH 12
#define THEMIS_AUTH_SYM_AAD_LENGTH 0
#define THEMIS_AUTH_SYM_AUTH_TAG_LENGTH 16

#define THEMIS_SYM_KDF_KEY_LABEL "Themis secure cell message key"
#define THEMIS_SYM_KDF_IV_LABEL "Themis secure cell message iv"

themis_status_t themis_sym_kdf(const uint8_t* master_key,
			       const size_t master_key_length,
			       const char* label,
			       const uint8_t* context,
			       const size_t context_length,
			       uint8_t* key,
			       size_t key_length){
  HERMES_CHECK_PARAM(master_key!=NULL && master_key_length!=0);
  HERMES_CHECK_PARAM(context!=NULL && context_length!=0);
  soter_kdf_context_buf_t ctx={context, context_length};
  HERMES_CHECK(soter_kdf(master_key, master_key_length, label, &ctx, 1, key, key_length)==HERMES_SUCCESS);
}

themis_status_t themis_auth_sym_plain_encrypt(uint32_t alg,
					      const uint8_t* key,
					      const size_t key_length,
					      const uint8_t* iv,
					      const size_t iv_length,
					      const uint8_t* aad,
					      const size_t aad_length,
					      const uint8_t* message,
					      const size_t message_length,
					      uint8_t* encrypted_message,
					      size_t* encrypted_message_length,
					      uint8_t* auth_tag,
					      size_t* auth_tag_length){
  soter_sym_ctx_t *ctx = soter_sym_aead_encrypt_create(alg, key, key_length, NULL,0,iv, iv_length);
  HERMES_CHECK(ctx!=NULL);
  if(aad!=NULL || aad_length!=0){
    HERMES_CHECK_FREE(soter_sym_aead_encrypt_aad(ctx, aad, aad_length)==HERMES_SUCCESS, ctx);
  }      
  HERMES_CHECK_FREE(soter_sym_aead_encrypt_update(ctx, message, message_length, encrypted_message, encrypted_message_length)==HERMES_SUCCESS, ctx);
  HERMES_CHECK_FREE(soter_sym_aead_encrypt_final(ctx, auth_tag, auth_tag_length)==HERMES_SUCCESS, ctx);
  HERMES_CHECK_FREE(soter_sym_aead_encrypt_destroy(ctx)==HERMES_SUCCESS, ctx);
  return HERMES_SUCCESS;
}

themis_status_t themis_auth_sym_plain_decrypt(uint32_t alg,
					      const uint8_t* key,
					      const size_t key_length,
					      const uint8_t* iv,
					      const size_t iv_length,
					      const uint8_t* aad,
					      const size_t aad_length,
					      const uint8_t* encrypted_message,
					      const size_t encrypted_message_length,
					      uint8_t* message,
					      size_t* message_length,
					      const uint8_t* auth_tag,
					      const size_t auth_tag_length){
  soter_sym_ctx_t *ctx = soter_sym_aead_decrypt_create(alg, key, key_length, NULL, 0, iv, iv_length);
  HERMES_CHECK(ctx!=NULL)
  if(aad!=NULL || aad_length!=0){
    HERMES_CHECK_FREE(soter_sym_aead_decrypt_aad(ctx, aad, aad_length)==HERMES_SUCCESS, ctx);
  }      
  HERMES_CHECK_FREE(soter_sym_aead_decrypt_update(ctx, encrypted_message, encrypted_message_length, message, message_length)==HERMES_SUCCESS, ctx);
  HERMES_CHECK_FREE(soter_sym_aead_decrypt_final(ctx, auth_tag, auth_tag_length)==HERMES_SUCCESS, ctx);
  HERMES_CHECK_FREE(soter_sym_aead_decrypt_destroy(ctx)==HERMES_SUCCESS, ctx);
  return HERMES_SUCCESS;
}

themis_status_t themis_sym_plain_encrypt(uint32_t alg,
					 const uint8_t* key,
					 const size_t key_length,
					 const uint8_t* iv,
					 const size_t iv_length,
					 const uint8_t* message,
					 const size_t message_length,
					 uint8_t* encrypted_message,
					 size_t* encrypted_message_length){
  soter_sym_ctx_t *ctx = soter_sym_encrypt_create(alg, key, key_length, NULL,0,iv, iv_length);
  HERMES_CHECK(ctx!=NULL);
  size_t add_length=(*encrypted_message_length);
  HERMES_CHECK_FREE(soter_sym_encrypt_update(ctx, message, message_length, encrypted_message, encrypted_message_length)==HERMES_SUCCESS, ctx);
  add_length-=(*encrypted_message_length);
  HERMES_CHECK_FREE(soter_sym_encrypt_final(ctx, encrypted_message+(*encrypted_message_length), &add_length)==HERMES_SUCCESS, ctx);
  (*encrypted_message_length)+=add_length;
  HERMES_CHECK_FREE(soter_sym_encrypt_destroy(ctx)==HERMES_SUCCESS, ctx);
  return HERMES_SUCCESS;
}

themis_status_t themis_sym_plain_decrypt(uint32_t alg,
					      const uint8_t* key,
					      const size_t key_length,
					      const uint8_t* iv,
					      const size_t iv_length,
					      const uint8_t* encrypted_message,
					      const size_t encrypted_message_length,
					      uint8_t* message,
					      size_t* message_length){
  soter_sym_ctx_t *ctx = soter_sym_decrypt_create(alg, key, key_length, NULL, 0, iv, iv_length);
  HERMES_CHECK(ctx!=NULL);
  size_t add_length=(*message_length);
  HERMES_CHECK_FREE(soter_sym_decrypt_update(ctx, encrypted_message, encrypted_message_length, message, message_length)==HERMES_SUCCESS, ctx);
  add_length-=(*message_length);
  HERMES_CHECK_FREE(soter_sym_decrypt_final(ctx, message+(*message_length), &add_length)==HERMES_SUCCESS, ctx);
  (*message_length)+=add_length;
  HERMES_CHECK_FREE(soter_sym_decrypt_destroy(ctx)==HERMES_SUCCESS, ctx);
  return HERMES_SUCCESS;
}


typedef struct themis_auth_sym_message_hdr_type{
  uint32_t alg;
  uint32_t iv_length;
  uint32_t aad_length;
  uint32_t auth_tag_length;
  uint32_t message_length;
} themis_auth_sym_message_hdr_t; 

themis_status_t themis_auth_sym_encrypt_message_(const uint8_t* key,
						 const size_t key_length,
						 const uint8_t* message,
						 const size_t message_length,
						 const uint8_t* in_context,
						 const size_t in_context_length,
						 uint8_t* out_context,
						 size_t* out_context_length,
						 uint8_t* encrypted_message,
						 size_t* encrypted_message_length){
  if(in_context!=NULL && in_context_length!=0){
    HERMES_CHECK_PARAM(in_context_length>THEMIS_AUTH_SYM_IV_LENGTH+THEMIS_AUTH_SYM_AAD_LENGTH);
  }
  if(encrypted_message==NULL || (*encrypted_message_length)<message_length || out_context==NULL || (*out_context_length)<(sizeof(themis_auth_sym_message_hdr_t)+THEMIS_AUTH_SYM_IV_LENGTH+THEMIS_AUTH_SYM_AAD_LENGTH+THEMIS_AUTH_SYM_AUTH_TAG_LENGTH)){
    (*encrypted_message_length)=message_length;
    (*out_context_length)=(sizeof(themis_auth_sym_message_hdr_t)+THEMIS_AUTH_SYM_IV_LENGTH+THEMIS_AUTH_SYM_AAD_LENGTH+THEMIS_AUTH_SYM_AUTH_TAG_LENGTH);
    return HERMES_BUFFER_TOO_SMALL;
  }
  (*encrypted_message_length)=message_length;
  (*out_context_length)=(sizeof(themis_auth_sym_message_hdr_t)+THEMIS_AUTH_SYM_IV_LENGTH+THEMIS_AUTH_SYM_AAD_LENGTH+THEMIS_AUTH_SYM_AUTH_TAG_LENGTH);
  themis_auth_sym_message_hdr_t* hdr=(themis_auth_sym_message_hdr_t*)out_context;
  uint8_t* iv=out_context+sizeof(themis_auth_sym_message_hdr_t);
  uint8_t* aad=iv+THEMIS_AUTH_SYM_IV_LENGTH;
  uint8_t* auth_tag=aad+THEMIS_AUTH_SYM_AAD_LENGTH;
  if(in_context!=NULL && in_context_length!=0){
    memcpy(iv, in_context, THEMIS_AUTH_SYM_IV_LENGTH);
    //    memcpy(aad,in_context+THEMIS_AUTH_SYM_IV_LENGTH, THEMIS_AUTH_SYM_AAD_LENGTH);    
  }
  else{
    HERMES_CHECK(soter_rand(iv, THEMIS_AUTH_SYM_IV_LENGTH)==HERMES_SUCCESS);
    //HERMES_CHECK(soter_rand(aad,THEMIS_AUTH_SYM_AAD_LENGTH)==HERMES_SUCCESS);
  }
  hdr->alg=THEMIS_AUTH_SYM_ALG;
  hdr->iv_length=THEMIS_AUTH_SYM_IV_LENGTH;
  hdr->aad_length=THEMIS_AUTH_SYM_AAD_LENGTH;
  hdr->auth_tag_length=THEMIS_AUTH_SYM_AUTH_TAG_LENGTH;
  hdr->message_length=message_length;
  size_t auth_tag_length=THEMIS_AUTH_SYM_AUTH_TAG_LENGTH;
  HERMES_CHECK(themis_auth_sym_plain_encrypt(THEMIS_AUTH_SYM_ALG, key, key_length, iv, THEMIS_AUTH_SYM_IV_LENGTH, aad, THEMIS_AUTH_SYM_AAD_LENGTH, message, message_length, encrypted_message, encrypted_message_length, auth_tag, &auth_tag_length)==HERMES_SUCCESS && auth_tag_length==THEMIS_AUTH_SYM_AUTH_TAG_LENGTH);
  return HERMES_SUCCESS;
}

themis_status_t themis_auth_sym_encrypt_message(const uint8_t* key,
						 const size_t key_length,
						 const uint8_t* message,
						 const size_t message_length,
						 const uint8_t* in_context,
						 const size_t in_context_length,
						 uint8_t* out_context,
						 size_t* out_context_length,
						 uint8_t* encrypted_message,
						 size_t* encrypted_message_length){
  uint8_t key_[THEMIS_SYM_KEY_LENGTH/8];
  HERMES_CHECK_PARAM(message!=NULL && message_length!=0);
  HERMES_STATUS_CHECK(themis_sym_kdf(key,key_length, THEMIS_SYM_KDF_KEY_LABEL, (uint8_t*)(&message_length), sizeof(message_length), key_, sizeof(key_)),HERMES_SUCCESS);
  return themis_auth_sym_encrypt_message_(key_, sizeof(key_), message, message_length, in_context, in_context_length, out_context, out_context_length, encrypted_message, encrypted_message_length);
}
themis_status_t themis_auth_sym_decrypt_message_(const uint8_t* key,
						const size_t key_length,
						const uint8_t* context,
						const size_t context_length,
						const uint8_t* encrypted_message,
						const size_t encrypted_message_length,
						uint8_t* message,
						size_t* message_length){
  HERMES_CHECK_PARAM(context_length>sizeof(themis_auth_sym_message_hdr_t));
  themis_auth_sym_message_hdr_t* hdr=(themis_auth_sym_message_hdr_t*)context;
  if(message==NULL || (*message_length)<hdr->message_length){
    (*message_length)=hdr->message_length;
    return HERMES_BUFFER_TOO_SMALL;
  }
  (*message_length)=hdr->message_length;
  HERMES_CHECK_PARAM(encrypted_message_length>=hdr->message_length);
  HERMES_CHECK_PARAM(context_length >= (sizeof(themis_auth_sym_message_hdr_t)+hdr->iv_length+hdr->aad_length+hdr->auth_tag_length));
  const uint8_t* iv=context+sizeof(themis_auth_sym_message_hdr_t);
  const uint8_t* aad=iv+hdr->iv_length;
  const uint8_t* auth_tag=aad+hdr->aad_length;
  HERMES_CHECK(themis_auth_sym_plain_decrypt(hdr->alg, key, key_length, iv, hdr->iv_length, aad, hdr->aad_length, encrypted_message, hdr->message_length, message, message_length, auth_tag, hdr->auth_tag_length)==HERMES_SUCCESS);
  return HERMES_SUCCESS;
}

themis_status_t themis_auth_sym_decrypt_message(const uint8_t* key,
						const size_t key_length,
						const uint8_t* context,
						const size_t context_length,
						const uint8_t* encrypted_message,
						const size_t encrypted_message_length,
						uint8_t* message,
						size_t* message_length){
  uint8_t key_[THEMIS_SYM_KEY_LENGTH/8];
  HERMES_CHECK_PARAM(context!=NULL && context_length!=0);
  HERMES_STATUS_CHECK(themis_sym_kdf(key,key_length, THEMIS_SYM_KDF_KEY_LABEL, (uint8_t*)(&encrypted_message_length), sizeof(encrypted_message_length), key_, sizeof(key_)),HERMES_SUCCESS);
  return themis_auth_sym_decrypt_message_(key_, sizeof(key_), context, context_length, encrypted_message, encrypted_message_length, message, message_length);
}
#define THEMIS_SYM_ALG (SOTER_SYM_AES_CTR|THEMIS_SYM_KEY_LENGTH)
#define THEMIS_SYM_IV_LENGTH 16

typedef struct themis_sym_message_hdr_type{
  uint32_t alg;
  uint32_t iv_length;
  uint32_t message_length;
} themis_sym_message_hdr_t;

themis_status_t themis_sym_encrypt_message_(const uint8_t* key,
					   const size_t key_length,
					   const uint8_t* message,
					   const size_t message_length,
					   const uint8_t* in_context,
					   const size_t in_context_length,
					   uint8_t* out_context,
					   size_t* out_context_length,
					   uint8_t* encrypted_message,
					   size_t* encrypted_message_length){
  if(in_context!=NULL && in_context_length!=0){
    HERMES_CHECK_PARAM(in_context_length>THEMIS_SYM_IV_LENGTH);
  }
  if(encrypted_message==NULL || (*encrypted_message_length)<message_length || out_context==NULL  || (*out_context_length)<(sizeof(themis_sym_message_hdr_t)+THEMIS_SYM_IV_LENGTH)){
    (*out_context_length)=(sizeof(themis_sym_message_hdr_t)+THEMIS_SYM_IV_LENGTH);
    (*encrypted_message_length)=message_length;
    return HERMES_BUFFER_TOO_SMALL;
  }
  (*encrypted_message_length)=message_length;
  (*out_context_length)=(sizeof(themis_sym_message_hdr_t)+THEMIS_SYM_IV_LENGTH);
  themis_sym_message_hdr_t* hdr=(themis_sym_message_hdr_t*)out_context;
  uint8_t* iv=out_context+sizeof(themis_sym_message_hdr_t);
  if(in_context!=NULL && in_context_length!=0){
    memcpy(iv, in_context, THEMIS_SYM_IV_LENGTH);
  }
  else{
    HERMES_CHECK(soter_rand(iv, THEMIS_SYM_IV_LENGTH)==HERMES_SUCCESS);
  }
  hdr->alg=THEMIS_SYM_ALG;
  hdr->iv_length=THEMIS_AUTH_SYM_IV_LENGTH;
  hdr->message_length=message_length;
  HERMES_CHECK(themis_sym_plain_encrypt(THEMIS_AUTH_SYM_ALG, key, key_length, iv, THEMIS_SYM_IV_LENGTH, message, message_length, encrypted_message, (size_t*)(&(hdr->message_length)))==HERMES_SUCCESS);  
  return HERMES_SUCCESS;
}

themis_status_t themis_sym_encrypt_message(const uint8_t* key,
					   const size_t key_length,
					   const uint8_t* message,
					   const size_t message_length,
					   const uint8_t* in_context,
					   const size_t in_context_length,
					   uint8_t* out_context,
					   size_t* out_context_length,
					   uint8_t* encrypted_message,
					   size_t* encrypted_message_length){
  uint8_t key_[THEMIS_SYM_KEY_LENGTH/8];
  HERMES_CHECK(themis_sym_kdf(key,key_length, THEMIS_SYM_KDF_KEY_LABEL, (uint8_t*)(&message_length), sizeof(message_length), key_, sizeof(key_))==HERMES_SUCCESS);
  return themis_sym_encrypt_message_(key_, sizeof(key_), message,message_length,in_context,in_context_length,out_context,out_context_length,encrypted_message,encrypted_message_length);
}

themis_status_t themis_sym_decrypt_message_(const uint8_t* key,
					    const size_t key_length,
					    const uint8_t* context,
					    const size_t context_length,
					    const uint8_t* encrypted_message,
					    const size_t encrypted_message_length,
					    uint8_t* message,
					    size_t* message_length){
  HERMES_CHECK_PARAM(context_length>sizeof(themis_sym_message_hdr_t));
  themis_sym_message_hdr_t* hdr=(themis_sym_message_hdr_t*)context;
  HERMES_CHECK_PARAM(encrypted_message_length>=hdr->message_length);
  HERMES_CHECK_PARAM(context_length >= (sizeof(themis_sym_message_hdr_t)+hdr->iv_length));
  if((*message_length)<hdr->message_length){
    (*message_length)=hdr->message_length;
    return HERMES_BUFFER_TOO_SMALL;
  }
  const uint8_t* iv=context+sizeof(themis_sym_message_hdr_t);
  HERMES_CHECK(themis_sym_plain_decrypt(hdr->alg, key, key_length, iv, hdr->iv_length, encrypted_message, hdr->message_length, message, message_length)==HERMES_SUCCESS);
  return HERMES_SUCCESS;
}

themis_status_t themis_sym_decrypt_message(const uint8_t* key,
					   const size_t key_length,
					   const uint8_t* context,
					   const size_t context_length,
					   const uint8_t* encrypted_message,
					   const size_t encrypted_message_length,
					   uint8_t* message,
					   size_t* message_length){
  uint8_t key_[THEMIS_SYM_KEY_LENGTH/8];
  HERMES_CHECK(themis_sym_kdf(key,key_length, THEMIS_SYM_KDF_KEY_LABEL, (uint8_t*)(&message_length), sizeof(message_length), key_, sizeof(key_))==HERMES_SUCCESS);
  return themis_sym_decrypt_message_(key_, sizeof(key_),context,context_length,encrypted_message,encrypted_message_length,message,message_length);
}

themis_status_t themis_sym_encrypt_message_u_(const uint8_t* key,
					     const size_t key_length,
					     const uint8_t* message,
					     const size_t message_length,
					     const uint8_t* context,
					     const size_t context_length,
					     uint8_t* encrypted_message,
					     size_t* encrypted_message_length){
  HERMES_CHECK_PARAM(context!=NULL && context_length!=0);
  if((*encrypted_message_length)<message_length){
    (*encrypted_message_length)=message_length;
    return HERMES_BUFFER_TOO_SMALL;
  }
  (*encrypted_message_length)=message_length;
  uint8_t iv[THEMIS_SYM_IV_LENGTH];
  HERMES_CHECK(themis_sym_kdf(key,key_length, THEMIS_SYM_KDF_IV_LABEL, context, context_length, iv, THEMIS_SYM_IV_LENGTH)==HERMES_SUCCESS);
  HERMES_CHECK(themis_sym_plain_encrypt(THEMIS_SYM_ALG, key, key_length, iv, THEMIS_SYM_IV_LENGTH, message, message_length, encrypted_message, encrypted_message_length)==HERMES_SUCCESS);  
  return HERMES_SUCCESS;
}

themis_status_t themis_sym_encrypt_message_u(const uint8_t* key,
					     const size_t key_length,
					     const uint8_t* context,
					     const size_t context_length,
					     const uint8_t* message,
					     const size_t message_length,
					     uint8_t* encrypted_message,
					     size_t* encrypted_message_length){
  uint8_t key_[THEMIS_SYM_KEY_LENGTH/8];
  HERMES_CHECK(themis_sym_kdf(key,key_length, THEMIS_SYM_KDF_KEY_LABEL, (uint8_t*)(&message_length), sizeof(message_length), key_, sizeof(key_))==HERMES_SUCCESS);  
  return themis_sym_encrypt_message_u_(key_, sizeof(key_), message,message_length,context,context_length,encrypted_message,encrypted_message_length);
}

themis_status_t themis_sym_decrypt_message_u_(const uint8_t* key,
					   const size_t key_length,
					   const uint8_t* context,
					   const size_t context_length,
					   const uint8_t* encrypted_message,
					   const size_t encrypted_message_length,
					   uint8_t* message,
					   size_t* message_length){
  HERMES_CHECK_PARAM(context!=NULL && context_length!=0);
  if((*message_length)<encrypted_message_length){
    (*message_length)=encrypted_message_length;
    return HERMES_BUFFER_TOO_SMALL;
  }
  (*message_length)=encrypted_message_length;
  uint8_t iv[THEMIS_SYM_IV_LENGTH];
  HERMES_CHECK(themis_sym_kdf(key,key_length, THEMIS_SYM_KDF_IV_LABEL, context, context_length, iv, THEMIS_SYM_IV_LENGTH)==HERMES_SUCCESS);
  HERMES_CHECK(themis_sym_plain_decrypt(THEMIS_SYM_ALG, key, key_length, iv, THEMIS_SYM_IV_LENGTH, encrypted_message, encrypted_message_length, message, message_length)==HERMES_SUCCESS);
  return HERMES_SUCCESS;
}

themis_status_t themis_sym_decrypt_message_u(const uint8_t* key,
					     const size_t key_length,
					     const uint8_t* context,
					     const size_t context_length,
					     const uint8_t* encrypted_message,
					     const size_t encrypted_message_length,
					     uint8_t* message,
					     size_t* message_length){
  uint8_t key_[THEMIS_SYM_KEY_LENGTH/8];
  HERMES_CHECK(themis_sym_kdf(key,key_length, THEMIS_SYM_KDF_KEY_LABEL, (uint8_t*)(&message_length), sizeof(message_length), key_, sizeof(key_))==HERMES_SUCCESS);  
  return themis_sym_decrypt_message_u_(key_,sizeof(key_),context,context_length,encrypted_message,encrypted_message_length,message,message_length);
}
